#include "ssd1306.h"
#include "linux/i2c.h"
#include "linux/module.h"
#include "ssd1306_mdev.h"

static int ssd1306_write_byte(struct i2c_client *client, u8 control_byte,
                              u8 data) {
    struct i2c_msg msg;
    int ret;
    u8 buf[2];
    buf[0] = control_byte;
    buf[1] = data;

    msg.addr = client->addr;
    msg.flags = 0; // Write operation
    msg.len = sizeof(buf);
    msg.buf = buf;

    ret = i2c_transfer(client->adapter, &msg, 1);
    if (ret < 1) {
        dev_err(&client->dev, "Failed to write byte: %d\n", ret);
        return ret;
    }
    return 0;
}

static int ssd1306_write_command(struct i2c_client *client, u8 cmd) {
    return ssd1306_write_byte(client, SSD1306_COMMAND, cmd);
}

static int ssd1306_device_init(struct i2c_client *client) {
    int ret = 0;
    ret |= ssd1306_write_command(client, 0xAE); /* Display OFF */
    ret |= ssd1306_write_command(client, 0x20); /* Set Memory Addressing Mode */
    ret |= ssd1306_write_command(client, 0x00); /* Horizontal Addressing Mode */
    ret |= ssd1306_write_command(client, 0xB0); /* Set Page Start Address 0 */
    ret |= ssd1306_write_command(client, 0xC8); /* COM Output Scan Direction */
    ret |= ssd1306_write_command(client, 0x00); /* Set low column address */
    ret |= ssd1306_write_command(client, 0x10); /* Set high column address */
    ret |= ssd1306_write_command(client, 0x40); /* Set start line address */
    ret |= ssd1306_write_command(client, 0x81); /* Set contrast control */
    ret |= ssd1306_write_command(client, 0xFF);
    ret |= ssd1306_write_command(client, 0xA1); /* Set segment re-map 0..127 */
    ret |= ssd1306_write_command(client, 0xA6); /* Normal display */
    ret |= ssd1306_write_command(client, 0xA8); /* Set multiplex ratio */
    ret |= ssd1306_write_command(client, 0x3F);
    ret |= ssd1306_write_command(client, 0xA4); /* Output RAM to display */
    ret |= ssd1306_write_command(client, 0xD3); /* Set display offset */
    ret |= ssd1306_write_command(client, 0x00); /* No offset */
    ret |= ssd1306_write_command(client, 0xD5); /* Set clock divide ratio */
    ret |= ssd1306_write_command(client, 0xF0);
    ret |= ssd1306_write_command(client, 0xD9); /* Set pre-charge period */
    ret |= ssd1306_write_command(client, 0x22);
    ret |= ssd1306_write_command(client, 0xDA); /* Set COM pins config */
    ret |= ssd1306_write_command(client, 0x12);
    ret |= ssd1306_write_command(client, 0xDB); /* Set vcomh */
    ret |= ssd1306_write_command(client, 0x20); /* 0.77xVcc */
    ret |= ssd1306_write_command(client, 0x8D); /* DC-DC enable */
    ret |= ssd1306_write_command(client, 0x14);
    ret |= ssd1306_write_command(client, 0xAF); /* Display ON */

    if (ret != 0) {
        dev_err(&client->dev, "SSD1306 Init Failed\n");
        return -EIO;
    }
    return 0;
}

int ssd1306_write_data_to_buf(struct i2c_client *client, const u8 *data,
                              size_t len) {
    struct i2c_msg msg;
    u8 *buf;
    int ret;

    if (!client || !data)
        return -EINVAL;
    if (len == 0)
        return 0;

    buf = kzalloc(len + 1, GFP_KERNEL);
    if (!buf)
        return -ENOMEM;
    // 写入数据寄存器
    buf[0] = SSD1306_DATA;
    // 拼接data到buf上
    memcpy(&buf[1], data, len);
    // 初始化msg结构体
    msg.addr = client->addr;
    msg.flags = 0;
    msg.buf = buf;
    msg.len = len + 1;
    // 发送msg
    ret = i2c_transfer(client->adapter, &msg, 1);
    // 释放buf内存
    kfree(buf);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to write data: %d\n", ret);
        return -EIO;
    }
    return 0;
}
// probe函数
static int ssd1306_probe(struct i2c_client *client) {
    struct ssd1306_data *data;
    int ret;

    dev_info(&client->dev, "=====SSD1306 OLED Display Detected=====\n");
    dev_info(&client->dev, "I2C Address 0x%02x, Adapter %s (bus i2c-%d)\n",
             client->addr, client->adapter->name, client->adapter->nr);

    // 分配设备数据结构内存
    data = devm_kzalloc(&client->dev, sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    // 私有数据i2c_client指针指向当前设备
    data->client = client;
    // 初始化mutex
    mutex_init(&data->lock);
    // 把私有数据结构指针保存到i2c_client中，方便后续使用
    i2c_set_clientdata(client, data);
    // 初始化设备
    mutex_lock(&data->lock);
    ret = ssd1306_device_init(client);
    mutex_unlock(&data->lock);
    if (ret) {
        dev_err(&client->dev, "Failed to initialize SSD1306 device\n");
        return ret;
    }
    // 注册misc_device
    ret = ssd1306_misc_register(data);
    dev_info(&client->dev, "=====SSD1306 OLED Probing Success=====");
    return 0;
}

static int ssd1306_remove(struct i2c_client *client) {
    struct ssd1306_data *data = i2c_get_clientdata(client);

    ssd1306_misc_deregister(data);
    dev_info(&client->dev, "=====SSD1306 OLED Removed=====\n");
    return 0;
}

static const struct i2c_device_id ssd1306_id_table[] = {
    {SSD1306_NAME, 0},
    {},
};
MODULE_DEVICE_TABLE(i2c, ssd1306_id_table);

static const struct of_device_id ssd1306_of_match[] = {
    {.compatible = "ssd1306"},
    {},
};
MODULE_DEVICE_TABLE(of, ssd1306_of_match);

static struct i2c_driver ssd1306_driver = {
    .driver =
        {
            .name = SSD1306_NAME,
            .of_match_table = ssd1306_of_match,
        },
    .probe_new = ssd1306_probe,
    .remove = ssd1306_remove,
    .id_table = ssd1306_id_table,
};

module_i2c_driver(ssd1306_driver);

MODULE_AUTHOR("cybersyn");
MODULE_DESCRIPTION("SSD1306 OLED Display Driver Demo");
MODULE_LICENSE("GPL");
