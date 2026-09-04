#include "ds3231.h"
#include "ds3231_misc.h"
#include "linux/dev_printk.h"
#include "linux/i2c.h"
#include "linux/interrupt.h"
#include "linux/module.h"
#include "linux/of_irq.h"
// BCD 字节转普通十进制整数
uint8_t bcd_to_dec(uint8_t bcd) {
    return ((bcd >> 4) & 0x0F) * 10 + (bcd & 0x0F);
}

// 普通十进制整数转 BCD 字节
uint8_t dec_to_bcd(uint8_t dec) { return ((dec / 10) << 4) | (dec % 10); }
// 写寄存器
static int ds3231_write_reg(struct i2c_client *client, u8 address, u8 data) {
    s32 ret = i2c_smbus_write_byte_data(client, address, data);
    if (ret < 0) {
        dev_err(&client->dev, "i2c_smbus write byte failed: %d", ret);
        return ret;
    }
    return 0;
}
// 读寄存器
static int ds3231_read_reg(struct i2c_client *client, u8 address, u8 *data) {
    s32 ret = i2c_smbus_read_byte_data(client, address);
    if (ret < 0) {
        dev_err(&client->dev, "i2c_smbus read byte failed: %d", ret);
        return ret;
    }
    *data = (u8)ret;
    return 0;
}
// 写一组寄存器（最多 8 字节数据 + 1 字节地址，栈上分配）
#define DS3231_WRITE_REGS_MAX 8
static int ds3231_write_regs(struct i2c_client *client, u8 start, u8 *data,
                             size_t len) {
    u8 buf[DS3231_WRITE_REGS_MAX + 1];
    struct i2c_msg msg;
    int ret;

    if (len == 0 || len > DS3231_WRITE_REGS_MAX)
        return -EINVAL;

    buf[0] = start;
    memcpy(&buf[1], data, len);

    msg.addr = client->addr;
    msg.buf = buf;
    msg.flags = 0;
    msg.len = len + 1;

    ret = i2c_transfer(client->adapter, &msg, 1);
    if (ret < 1) {
        dev_err(&client->dev, "i2c_transfer write failed: %d", ret);
        return -EIO;
    }
    return 0;
}
// 读一组寄存器
static int ds3231_read_regs(struct i2c_client *client, u8 start, u8 *data,
                            size_t len) {
    int ret;
    struct i2c_msg msgs[2] = {{
                                  .addr = client->addr,
                                  .len = 1,
                                  .flags = 0,
                                  .buf = &start,
                              },
                              {
                                  .addr = client->addr,
                                  .len = len,
                                  .flags = I2C_M_RD,
                                  .buf = data,
                              }};
    ret = i2c_transfer(client->adapter, msgs, 2);
    if (ret < 2) {
        dev_err(&client->dev, "i2c_transfer read failed: %d", ret);
        return -EIO;
    }
    return 0;
}
// 读时间
int ds3231_read_time(struct i2c_client *client, struct ds3231_t *time) {
    u8 buf[3];
    int ret = ds3231_read_regs(client, 0x00, buf, 3);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to read time: %d", ret);
        return ret;
    }
    /*dev_info(&client->dev, "raw time regs: sec=0x%02x min=0x%02x hour=0x%02x",
             buf[0], buf[1], buf[2]);*/

    time->time.second = bcd_to_dec(buf[0] & 0x7F);
    time->time.minutes = bcd_to_dec(buf[1] & 0x7F);
    time->time.hour = bcd_to_dec(buf[2] & 0x3f);

    /*dev_info(&client->dev, "decoded time: %02u:%02u:%02u", time->time.hour,
             time->time.minutes, time->time.second);*/
    if (time->time.hour > 23) {
        dev_err(&client->dev, "Invalid hour value: %d", time->time.hour);
        return -EIO;
    }
    if (time->time.minutes > 59) {
        dev_err(&client->dev, "Invalid minute value: %d", time->time.minutes);
        return -EIO;
    }
    if (time->time.second > 59) {
        dev_err(&client->dev, "Invalid second value: %d", time->time.second);
        return -EIO;
    }
    return 0;
}
// 写时间
int ds3231_write_time(struct i2c_client *client, struct ds3231_t *time,
                      u8 *data) {
    u8 hw_buf[3];
    int ret;
    if (!data)
        return -EINVAL;

    // 转换为硬件顺序：秒、分、时，并转 BCD
    hw_buf[0] = dec_to_bcd(data[2]); // 秒
    hw_buf[1] = dec_to_bcd(data[1]); // 分
    hw_buf[2] = dec_to_bcd(data[0]); // 时

    ret = ds3231_write_regs(client, 0x00, hw_buf, 3);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to write time: %d", ret);
        return ret;
    }

    // 同步更新软件时间（十进制）
    time->time.hour = data[0];
    time->time.minutes = data[1];
    time->time.second = data[2];

    return 0;
}
static int ds3231_device_init(struct i2c_client *client) {
    int ret;
    ret = ds3231_write_reg(client, 0x0E, 0x00);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to write control register: %d", ret);
        return ret;
    }

    ret = ds3231_write_reg(client, 0x0F, 0x00);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to write status register: %d", ret);
        return ret;
    }

    return 0;
}
static irqreturn_t ds3231_sqw_test_handler(int irq, void *dev_id) {
    struct ds3231_t *data = dev_id;
    u64 ts_start, ts_end;
    ts_start = ktime_get_ns();
    mutex_lock(&data->lock);
    if (ds3231_read_time(data->client, data) == 0) {
        data->updata_ready = true;
        wake_up_interruptible(&data->wq);
    }
    mutex_unlock(&data->lock);
    ts_end = ktime_get_ns();
    dev_dbg(&data->client->dev,
            "SQW interrupt: time updated to %02u:%02u:%02u, "
            "handler latency = %lld ns",
            data->time.hour, data->time.minutes, data->time.second,
            ts_end - ts_start);
    return IRQ_HANDLED;
}
static int ds3231_probe(struct i2c_client *client) {
    struct ds3231_t *data;
    int ret;
    int irq;

    dev_info(&client->dev, "========Probing DS3231 RTC device========");
    dev_info(&client->dev, "========I2C Address: 0x%02x========", client->addr);

    data = devm_kzalloc(&client->dev, sizeof(struct ds3231_t), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    data->client = client;
    // 初始化互斥锁
    mutex_init(&data->lock);
    // 初始化等待队列和状态
    init_waitqueue_head(&data->wq);
    data->updata_ready = false;

    i2c_set_clientdata(client, data);

    ret = ds3231_device_init(data->client);
    if (ret) {
        dev_err(&client->dev, "Failed to initialize device: %d", ret);
        return ret;
    }
    dev_info(&client->dev, "Device initialized successfully");
    /* ---------- 中断测试开始 ---------- */
    /* 从设备树获取中断号 */
    irq = of_irq_get(client->dev.of_node, 0);
    if (irq <= 0) {
        dev_err(&client->dev, "Failed to get IRQ, error %d\n", irq);
        return irq;
    }

    /* 注册一个简单的线程化中断处理函数（仅打印） */
    ret = devm_request_threaded_irq(
        &client->dev, irq, NULL, ds3231_sqw_test_handler,
        IRQF_TRIGGER_RISING | IRQF_ONESHOT, "ds3231_sqw", data);
    if (ret) {
        dev_err(&client->dev, "Failed to request IRQ %d: %d\n", irq, ret);
        return ret;
    }

    dev_info(&client->dev, "SQW interrupt registered, IRQ=%d\n", irq);
    ret = ds3231_misc_register(data);
    if (ret)
        return ret;

    dev_info(&client->dev, "========DS3231 probe complete========");
    return 0;
}
static int ds3231_remove(struct i2c_client *client) {
    struct ds3231_t *data = i2c_get_clientdata(client);
    ds3231_misc_unregister(data);
    dev_info(&client->dev, "DS3231 removed");
    return 0;
}

static const struct of_device_id ds3231_of_match[] = {{.compatible = "ds3231"},
                                                      {}};
MODULE_DEVICE_TABLE(of, ds3231_of_match);

static const struct i2c_device_id ds3231_id[] = {
    {DS3231_NAME, 0},
    {},
};
MODULE_DEVICE_TABLE(i2c, ds3231_id);

static struct i2c_driver ds3231_driver = {
    .driver =
        {
            .name = DS3231_NAME,
            .of_match_table = ds3231_of_match,
        },
    .probe_new = ds3231_probe,
    .remove = ds3231_remove,
    .id_table = ds3231_id,
};

module_i2c_driver(ds3231_driver);

MODULE_AUTHOR("cybersyn-stm");
MODULE_DESCRIPTION("DS3231 RTC Driver");
MODULE_LICENSE("GPL");
