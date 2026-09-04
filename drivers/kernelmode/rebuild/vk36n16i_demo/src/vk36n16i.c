#include "vk36n16i.h"

// 设置poll_ms参数，用于设置轮询时间
static unsigned int poll_ms = 20;
module_param(poll_ms, uint, 0644);

static const uint8_t key_value_table[16] = {1, 4, 7, 15, 2,  5,  8,  0,
                                            3, 6, 9, 12, 10, 13, 11, 14};

// 将十六进制值解码为对应键值
static int vk36n16i_decode_logic_key(uint16_t raw) {
    int bit;

    if (raw == 0) {
        return -1;
    }
    // 计算raw中第一个非零的位置
    // 键值：raw = 0x0001 则bit = 0;
    bit = __ffs(raw);
    if (bit >= 16 || bit < 0) {
        return -1;
    }

    return key_value_table[bit];
}
// vk36n16i_read读取键值数据
// vk36n16i设备读取键值，只需要写入设备地址，然后直接读取数据即可，无需写入寄存器地址
static int vk36n16i_read(struct i2c_client *client, uint16_t *data) {
    uint8_t buf[2]; // msg buffer
    struct i2c_msg msg = {
        .addr = client->addr,
        .buf = buf,
        .len = sizeof(buf),
        .flags = I2C_M_RD,
    };

    int ret;
    ret = i2c_transfer(client->adapter, &msg, 1);
    if (ret < 0) {
        dev_err(&client->dev, "i2c_transfer failed: %d\n", ret);
        return ret;
    }
    if (ret != 1) {
        dev_err(&client->dev, "i2c_transfer returned unexpected value: %d\n",
                ret);
        return -EIO;
    }

    *data = (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);

    return 0;
}
// *==============misc设备文件操作函数：open、read、poll==================*
// open，私有数据从misc设备结构体指针转换为vk36n16i_data结构体指针，并将其存储在file的private_data中，以便后续操作使用
static int vk36n16i_misc_open(struct inode *inode, struct file *file) {
    // 拿到misc设备结构体指针，进而拿到vk36n16i_data结构体指针
    struct miscdevice *m_dev = file->private_data;
    struct vk36n16i_data *data =
        container_of(m_dev, struct vk36n16i_data, miscdev);

    file->private_data = data;
    return 0;
}
// read, 读取键值并放到用户态缓冲区中
static ssize_t vk36n16i_misc_read(struct file *file, char __user *buf,
                                  size_t count, loff_t *ppos) {
    struct vk36n16i_data *data = file->private_data;
    u16 raw; // 键值数据
    int ret;
    int key_value;
    // 当用户态调用read时，判断用户态缓冲大小是否足够存放一个u16类型的键值数据，如果不足则返回错误
    if (count < sizeof(u16)) {
        return -EINVAL;
    }
    // 判断是否为阻塞模式，如果是阻塞模式则等待事件发生
    if (!(file->f_flags & O_NONBLOCK)) {
        // 等待事件函数，等待队列wait_queue,唤醒条件event_pending为真时唤醒
        // 使队列休眠，等待唤醒标志位
        ret = wait_event_interruptible(data->wait_queue, data->event_pending);
        // 如果等待过程中被信号中断，则返回ret
        if (ret)
            return ret;
    } // 如果非阻塞模式下，如果没有事件待处理则直接返回-EAGAIN错误
    else {
        if (!data->event_pending) {
            return -EAGAIN;
        }
    }
    // 消费事件 读取键值数据并发送到用户态缓冲区，重置事件待处理标志
    //  mutex互斥锁，保护数据访问，防止竞态条件
    mutex_lock(&data->lock);
    // 锁进程，读取键值数据
    data->event_pending = false; // 事件已处理，重置事件待处理标志
    raw = data->key_value;       // 从数据结构体中获取键值数据
    mutex_unlock(&data->lock);
    // 解码键值数据
    key_value = vk36n16i_decode_logic_key(raw);
    if (copy_to_user(buf, &key_value, sizeof(key_value))) {
        return -EFAULT;
    }
    // read函数返回值要求实际读取字节数
    return sizeof(key_value);
}
// poll, 轮询函数，等待事件发生并返回可读事件标志
static __poll_t vk36n16i_misc_poll(struct file *file,
                                   struct poll_table_struct *poll_table) {
    struct vk36n16i_data *data = file->private_data;
    __poll_t mask = 0;

    poll_wait(file, &data->wait_queue, poll_table);

    if (READ_ONCE(data->event_pending)) {
        mask |= POLLIN | POLLRDNORM; // 可读事件
    }

    return mask;
}
// misc设备文件操作函数：open、read、poll
static const struct file_operations vk36n16i_fops = {
    .owner = THIS_MODULE,
    .open = vk36n16i_misc_open,
    .read = vk36n16i_misc_read,
    .poll = vk36n16i_misc_poll,
};
// 轮询工作队列回调函数
static void vk36n16i_poll_workfn(struct work_struct *work) {
    struct vk36n16i_data *data =
        container_of(work, struct vk36n16i_data, poll_work.work);

    u16 raw;
    int ret;
    int key_value;

    mutex_lock(&data->lock);
    ret = vk36n16i_read(data->client, &raw);
    // 读取成功，判断是否有键值变化，如果有变化则更新数据并设置事件待处理标志
    if (!ret) {
        data->key_value = raw;
        data->last_key_value = raw;
        // 判断键值是否变化
        if (data->last_key_value != data->last_report) {
            data->last_report = data->last_key_value;
            // 事件更新标志位
            data->event_pending = true;
        }
    }
    key_value = vk36n16i_decode_logic_key(raw);
    mutex_unlock(&data->lock);
    // 唤醒队列,READ_ONCE安全读取标志位
    if (READ_ONCE(data->event_pending)) {
        wake_up_interruptible(&data->wait_queue);
    }
    // 将任务加入到轮询队列，msecs_to_jiffies 将毫秒转换为内核时间单位
    schedule_delayed_work(&data->poll_work, msecs_to_jiffies(poll_ms));
}
// 轮询采集层启动
static int vk36n16i_polling_misc_start(struct i2c_client *client) {
    struct vk36n16i_data *data = i2c_get_clientdata(client);
    int ret;
    // 初始化队列
    init_waitqueue_head(&data->wait_queue);
    // 初始化事件待处理标志
    data->event_pending = false;
    data->last_key_value = 0;
    data->last_report = 0;
    // 注册 misc 设备
    data->miscdev.minor = MISC_DYNAMIC_MINOR;
    data->miscdev.name = VK36N16I_NAME;
    data->miscdev.fops = &vk36n16i_fops;
    data->miscdev.parent = &client->dev;

    ret = misc_register(&data->miscdev);
    if (ret) {
        dev_err(&client->dev, "Failed to register misc device: %d\n", ret);
        return ret;
    }

    INIT_DELAYED_WORK(&data->poll_work, vk36n16i_poll_workfn);
    schedule_delayed_work(&data->poll_work, msecs_to_jiffies(poll_ms));

    dev_info(&client->dev, "polling started (%ums), /dev/%s, ready\n", poll_ms,
             VK36N16I_NAME);
    return 0;
}
// 轮询采集层停止
static void vk36n16i_polling_misc_stop(struct i2c_client *client) {
    struct vk36n16i_data *data = i2c_get_clientdata(client);

    cancel_delayed_work_sync(&data->poll_work);
    misc_deregister(&data->miscdev);
    dev_info(&client->dev, "Polling stopped and misc device deregistered\n");
}
// 端口初始化
static int __init duo_pinmux_init(struct i2c_client *client) {
    int ret;
    dev_info(&client->dev, "Init pinmux\n");
    dev_info(&client->dev, "Setting B12 as IIC1_SCL\n");
    ret = bash_execute("duo-pinmux -w B12/IIC1_SCL\n");
    if (ret != 0) {
        dev_err(&client->dev, "Failed to set B12 as IIC1_SCL\n");
        return ret;
    }
    dev_info(&client->dev, "Setting B12 as IIC1_SCL\n");
    ret = bash_execute("duo-pinmux -w B11/IIC1_SDA\n");
    if (ret != 0) {
        dev_err(&client->dev, "Failed to set B11 as IIC1_SDA\n");
        return ret;
    }

    dev_info(&client->dev, "Pinmux init completed\n");
    return 0;
}
// 创建sysfs属性文件，供用户态访问键值数据
static ssize_t key_value_show(struct device *dev, struct device_attribute *attr,
                              char *buf) {
    struct i2c_client *client = to_i2c_client(dev);
    struct vk36n16i_data *data = i2c_get_clientdata(client);
    u16 key_value;

    mutex_lock(&data->lock);
    key_value = data->key_value;
    mutex_unlock(&data->lock);
    return sysfs_emit(buf, "0x%04x\n", key_value);

    // return sprintf(buf, "0x%04x\n", key_value);
}
static DEVICE_ATTR_RO(key_value); // 只读属性
                                  //
// 设备probe函数，设备初始化入口
static int vk36n16i_probe(struct i2c_client *client) {
    struct vk36n16i_data *data;
    int ret;

    dev_info(&client->dev, "====== Probe Device =====\n");
    dev_info(&client->dev, "I2C address = 0x%02x adapter = %s (bus I2C-%d)\n",
             client->addr, client->adapter->name, client->adapter->nr);
    // 分配设备数据结构体内存
    // devm_kzalloc = device managed kernel zero allocate,
    // 自动管理内存生命周期，设备移除时自动释放 GFP : Get Free Pages,
    // GFP_KERNEL分配内核内存
    data = devm_kzalloc(&client->dev, sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;
    // 初始化引脚设置
    duo_pinmux_init(client);
    // 使私有数据的client指针指向当前i2c客户端结构体，方便后续访问
    data->client = client;
    mutex_init(&data->lock);
    // 注册私有数据结构体到i2c客户端，以便后续访问
    i2c_set_clientdata(client, data);
    // 创建设备文件，并创建sysfs属性文件，供用户态访问键值数据
    ret = device_create_file(&client->dev, &dev_attr_key_value);
    if (ret) {
        dev_err(&client->dev, "Failed to create sysfs attribute: %d\n", ret);
        return ret;
    }

    ret = vk36n16i_polling_misc_start(client);
    if (ret) {
        dev_err(&client->dev, "Failed to start polling misc device: %d\n", ret);
        return ret;
    }
    dev_info(&client->dev, "Probe successful\n");
    return 0;
}

// 设备remove函数，设备移除入口
static int vk36n16i_remove(struct i2c_client *client) {
    // 关闭轮询采集层，注销设备文件
    vk36n16i_polling_misc_stop(client);
    device_remove_file(&client->dev, &dev_attr_key_value);
    dev_info(&client->dev, "Device removed\n");
    return 0;
}
// 设备树匹配表
static const struct of_device_id vk36n16i_of_match[] = {
    {.compatible = "vk36n16i"},
};

// i2c设备ID表
static const struct i2c_device_id vk36n16i_of_i2c_id[] = {
    {VK36N16I_NAME, 0},
    {},
};

// i2c驱动结构体
static struct i2c_driver vk36n16i_driver = {
    .driver =
        {
            .name = VK36N16I_NAME,
            .of_match_table = vk36n16i_of_match,
        },
    .probe_new = vk36n16i_probe,
    .remove = vk36n16i_remove,
    .id_table = vk36n16i_of_i2c_id,
};

module_i2c_driver(vk36n16i_driver);

MODULE_AUTHOR("cybersyn");
MODULE_DESCRIPTION("Driver for VK36N16I I2C Keypad");
MODULE_LICENSE("GPL");
