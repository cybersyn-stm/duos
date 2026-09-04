/*
 * VK36N16I I2C 键盘驱动主文件
 * 主要功能：
 *  - 通过 I2C 读取 VK36N16I 键盘芯片的按键值
 *  - 通过 misc 设备提供字符设备接口 (/dev/vk36n16i)
 *  - 通过 sysfs 提供按键值只读属性
 *  - 采用定时轮询方式读取按键
 */

#include "vk36n16i.h"
#include "asm-generic/fcntl.h"
#include "duo_pinmux.h"
#include "linux/dev_printk.h"
#include "linux/export.h"
#include "linux/fs.h"
#include "linux/kern_levels.h"
#include "linux/miscdevice.h"
#include "linux/printk.h"
#include "linux/stddef.h"

#include <linux/bitops.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/sysfs.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

// 轮询间隔（毫秒），可通过模块参数 poll_ms 修改
static unsigned int poll_ms = 20;
module_param(poll_ms, uint, 0644); // 导出为模块参数，权限0644（rw-r--r--）
MODULE_PARM_DESC(poll_ms,
                 "Polling interval in milliseconds (default: 20)"); // 参数描述

// 按键值查找表（行列扫描转逻辑键值）
static const uint8_t key_value_table[16] = {1, 4, 7, 15, 2,  5,  8,  0,
                                            3, 6, 9, 12, 10, 13, 11, 14};

/* 轮询工作队列回调函数：定时读取按键值，唤醒等待队列 */
static void vk36n16i_poll_workfn(struct work_struct *work) {
    struct vk36n16i_data *data =
        container_of(work, struct vk36n16i_data, poll_work.work);

    u16 raw;
    int ret;
    int key_value;

    mutex_lock(&data->lock);
    ret = vk36n16i_read(data->client, &raw);

    if (!ret) {
        data->key_value = raw;
        data->last_raw = raw;

        // 检查是否有新事件
        if (data->last_raw != data->last_report) {
            data->last_report = data->last_raw;
            data->event_pending = true;
        }
    }
    key_value = vk36n16i_decode_logic_key(raw);
    // dev_info(&data->client->dev, "%d", key_value);
    mutex_unlock(&data->lock);

    // 唤醒等待队列
    if (READ_ONCE(data->event_pending)) {
        wake_up_interruptible(&data->wq);
    }

    // 重新调度下一次轮询
    schedule_delayed_work(&data->poll_work, msecs_to_jiffies(poll_ms));
}

/* 字符设备 open 回调，初始化 private_data */
static int vk36n16i_chr_open(struct inode *inode, struct file *file) {
    struct miscdevice *mdev = file->private_data;
    struct vk36n16i_data *data =
        container_of(mdev, struct vk36n16i_data, miscdev);

    file->private_data = data;
    return 0;
}

/* 字符设备 read 回调，读取按键值（阻塞/非阻塞） */
static ssize_t vk36n16i_chr_read(struct file *file, char __user *user_buf,
                                 size_t count, loff_t *ppos) {
    struct vk36n16i_data *data = file->private_data;
    u16 raw;
    int ret;
    int key_value;

    if (count < sizeof(raw))
        return -EINVAL;
    // 判断文件是否为阻塞模式，如果是阻塞模式且没有事件，则进入睡眠等待；如果是非阻塞模式且没有事件，则立即返回
    // -EAGAIN
    if (!(file->f_flags & O_NONBLOCK)) {
        ret = wait_event_interruptible(data->wq, data->event_pending);
        if (ret)
            return ret;
    } else {
        if (!data->event_pending)
            return -EAGAIN;
    }

    mutex_lock(&data->lock);
    data->event_pending = false;
    raw = data->last_raw;
    mutex_unlock(&data->lock);

    key_value = vk36n16i_decode_logic_key(raw);
    if (copy_to_user(user_buf, &key_value, sizeof(key_value)))
        return -EFAULT;
    printk("%d", key_value);

    return sizeof(key_value);
}

/* 字符设备 poll 回调，支持 select/poll 机制 */
static __poll_t vk36n16i_chr_poll(struct file *file, poll_table *wait) {
    struct vk36n16i_data *data = file->private_data;
    __poll_t mask = 0;

    poll_wait(file, &data->wq, wait);

    if (READ_ONCE(data->event_pending))
        mask |= POLLIN | POLLRDNORM;

    return mask;
}

// 字符设备操作集
static const struct file_operations vk36n16i_fops = {
    .owner = THIS_MODULE,
    .open = vk36n16i_chr_open,
    .read = vk36n16i_chr_read,
    .poll = vk36n16i_chr_poll,
    .llseek = no_llseek,
};

/* 启动轮询、注册 misc 设备 */
static int vk36n16i_polling_misc_start(struct i2c_client *client) {
    struct vk36n16i_data *data = i2c_get_clientdata(client);
    int ret;
    // 初始化等待队列
    init_waitqueue_head(&data->wq);
    // 初始化事件标志
    data->event_pending = false;
    data->last_raw = 0;
    data->last_report = 0;
    // 注册 misc 设备
    data->miscdev.minor = MISC_DYNAMIC_MINOR;
    data->miscdev.name = VK36N16I_NAME;
    data->miscdev.fops = &vk36n16i_fops;
    data->miscdev.parent = &client->dev;

    ret = misc_register(&data->miscdev);
    if (ret) {
        dev_err(&client->dev, "misc_register failed: %d\n", ret);
        return ret;
    }
    // 初始化并启动轮询工作队列
    INIT_DELAYED_WORK(&data->poll_work, vk36n16i_poll_workfn);
    schedule_delayed_work(&data->poll_work, msecs_to_jiffies(poll_ms));

    dev_info(&client->dev, "polling started (%ums), /dev/%s ready\n", poll_ms,
             VK36N16I_NAME);
    return 0;
}

/* 停止轮询、注销 misc 设备 */
static void vk36n16i_polling_misc_stop(struct i2c_client *client) {
    struct vk36n16i_data *data = i2c_get_clientdata(client);

    cancel_delayed_work_sync(&data->poll_work);
    misc_deregister(&data->miscdev);
    dev_info(&client->dev, "polling stopped, /dev/%s removed\n", VK36N16I_NAME);
}

/* duo-pinmux 初始化，配置 I2C 引脚复用 */
static int __init duo_pinmux_init(void) {
    int ret = 0;
    printk(KERN_INFO "Loading duo-pinmux kernel module\n");

    printk(KERN_INFO "Configuring B12 as IIC1_SCL\n");
    ret = execute_command("duo-pinmux -w B12/IIC1_SCL");
    if (ret != 0) {
        printk(KERN_ERR "Failed to configure B12\n");
        return ret;
    }

    printk(KERN_INFO "Configuring B11 as IIC1_SDA\n");
    ret = execute_command("duo-pinmux -w B11/IIC1_SDA");
    if (ret != 0) {
        printk(KERN_ERR "Failed to configure B11\n");
        return ret;
    }

    printk(KERN_INFO "duo-pinmux configuration completed successfully\n");
    return 0;
}

/* 通过 I2C 读取 2 字节原始数据 */
static int vk36n16i_read(struct i2c_client *client, uint16_t *data) {
    uint8_t buf[2];
    struct i2c_msg msg = {
        .addr = client->addr,
        .flags = I2C_M_RD,
        .len = sizeof(buf),
        .buf = buf,
    };
    int ret;

    // 只发送 1 条消息
    ret = i2c_transfer(client->adapter, &msg, 1);
    // ret < 0 没有发送msg
    if (ret < 0) {
        dev_err(&client->dev, "i2c_transfer failed: %d\n", ret);
        return ret;
    }
    // ret != 1 没有发送完整个msg
    if (ret != 1) {
        dev_err(&client->dev, "i2c_transfer short: %d\n", ret);
        return -EIO;
    }

    // 按协议拼接为 16 位数据
    *data = (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
    return 0;
}

/* 原始数据解码为逻辑按键值 */
static int vk36n16i_decode_logic_key(uint16_t raw) {
    int bit;

    if (raw == 0)
        return -1;

    bit = __ffs(raw); /* 0..15 */
    if (bit < 0 || bit >= 16)
        return -1;

    return key_value_table[bit];
}

/* sysfs 属性：key_value，显示当前按键值 */
static ssize_t key_value_show(struct device *dev, struct device_attribute *attr,
                              char *buf) {
    struct i2c_client *client = to_i2c_client(dev);
    struct vk36n16i_data *data = i2c_get_clientdata(client);
    uint16_t raw;
    int logic;
    int ret;

    mutex_lock(&data->lock);
    ret = vk36n16i_read(client, &raw);
    if (!ret)
        data->key_value = raw; /* cache raw */
    mutex_unlock(&data->lock);

    if (ret)
        return ret;

    logic = vk36n16i_decode_logic_key(raw);

    // 只输出逻辑按键值
    return sysfs_emit(buf, "%d\n", logic);
}

static DEVICE_ATTR_RO(key_value);

/* I2C 驱动 probe 回调，初始化设备 */
static int vk36n16i_probe(struct i2c_client *client) {
    struct vk36n16i_data *data;
    int ret;

    dev_info(&client->dev, "====== Probing %s START ======\n", VK36N16I_NAME);
    dev_info(&client->dev, "I2C addr=0x%02x adapter=%s(bus %d)\n", client->addr,
             client->adapter->name, client->adapter->nr);

    data = devm_kzalloc(&client->dev, sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;
    duo_pinmux_init();

    data->client = client;
    mutex_init(&data->lock);
    i2c_set_clientdata(client, data);

    ret = device_create_file(&client->dev, &dev_attr_key_value);
    if (ret) {
        dev_err(&client->dev, "device_create_file(key_value) failed: %d\n",
                ret);
        return ret;
    }

    ret = vk36n16i_polling_misc_start(client);
    if (ret) {
        dev_err(&client->dev, "vk36n16i_polling_misc_start failed: %d\n", ret);
        return ret;
    }
    dev_info(&client->dev, "%s probed successfully\n", VK36N16I_NAME);
    dev_info(&client->dev, "====== Probing %s DONE ======\n", VK36N16I_NAME);
    return 0;
}

/* I2C 驱动 remove 回调，清理资源 */
static int vk36n16i_remove(struct i2c_client *client) {
    vk36n16i_polling_misc_stop(client);
    device_remove_file(&client->dev, &dev_attr_key_value);
    dev_info(&client->dev, "%s removed\n", VK36N16I_NAME);
    return 0;
}

// 设备树匹配表
static const struct of_device_id vk36n16i_of_match[] = {
    {.compatible = "vk36n16i"}, {}};
MODULE_DEVICE_TABLE(of, vk36n16i_of_match);

// I2C 设备 ID 表
static const struct i2c_device_id vk36n16i_id[] = {{"vk36n16i", 0}, {}};
MODULE_DEVICE_TABLE(i2c, vk36n16i_id);

// I2C 驱动结构体
static struct i2c_driver vk36n16i_driver = {
    .driver =
        {
            .name = VK36N16I_NAME,
            .of_match_table = vk36n16i_of_match,
        },
    .probe_new = vk36n16i_probe, /* newer kernels prefer probe_new */
    .remove = vk36n16i_remove,
    .id_table = vk36n16i_id,
};

module_i2c_driver(vk36n16i_driver);

MODULE_AUTHOR("cybersyn-stm");
MODULE_DESCRIPTION("VK36N16I key reader driver (i2c_msg + sysfs)");
MODULE_LICENSE("GPL");
