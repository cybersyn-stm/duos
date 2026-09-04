#include "ssd1306_mdev.h"
#include "linux/fs.h"
#include "linux/ioctl.h"
#include "ssd1306.h"
// misc设备文件操作函数
static int ssd1306_misc_open(struct inode *inode, struct file *file) {
    // 重定位private_data指针，指向ssd1306_data结构体
    struct miscdevice *mdev = file->private_data;
    struct ssd1306_data *data;
    if (!mdev)
        return -ENODEV;

    data = container_of(mdev, struct ssd1306_data, miscdev);
    file->private_data = data;
    return 0;
}
static ssize_t ssd1306_misc_write(struct file *file, const char __user *buf,
                                  size_t count, loff_t *ppos) {
    // 获取ssd1306_data结构体指针
    struct ssd1306_data *data = file->private_data;
    u8 *to_kernel_buf;
    int ret;

    if (!data || !data->client)
        return -ENODEV;
    if (count == 0)
        return 0;
    if (count != SSD1306_FB_SIZE)
        return -EINVAL;
    // 分配用户控件内存
    to_kernel_buf = memdup_user(buf, count);
    if (IS_ERR(to_kernel_buf))
        return PTR_ERR(to_kernel_buf);

    mutex_lock(&data->lock);
    ret = ssd1306_write_data_to_buf(data->client, to_kernel_buf, count);
    mutex_unlock(&data->lock);

    kfree(to_kernel_buf);
    if (ret) {
        return ret;
    }

    return count;
}

static const struct file_operations ssd1306_fops = {
    .owner = THIS_MODULE,
    .open = ssd1306_misc_open,
    .write = ssd1306_misc_write,
    .llseek = no_llseek,
};

int ssd1306_misc_register(struct ssd1306_data *data) {
    int ret;

    if (!data || !data->client)
        return -EINVAL;

    data->miscdev.minor = MISC_DYNAMIC_MINOR;
    data->miscdev.name = SSD1306_NAME;
    data->miscdev.fops = &ssd1306_fops;
    data->miscdev.parent = &data->client->dev;

    ret = misc_register(&data->miscdev);
    if (ret) {
        dev_err(&data->client->dev, "Failed to register misc device: %d\n",
                ret);
    } else {
        dev_info(&data->client->dev,
                 "Misc device registered successfully: /dev/%s\n",
                 SSD1306_NAME);
    }

    return ret;
}

void ssd1306_misc_deregister(struct ssd1306_data *data) {
    if (!data)
        return;
    misc_deregister(&data->miscdev);
}
