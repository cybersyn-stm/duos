#include "ds3231_misc.h"
#include "ds3231.h"

#include "linux/err.h"
#include "linux/export.h"
#include "linux/fs.h"
#include "linux/miscdevice.h"
#include "linux/mutex.h"
#include "linux/poll.h"
#include "linux/string.h"
// ds3231 open
static int ds3231_misc_open(struct inode *inode, struct file *file) {
    struct miscdevice *mdev = file->private_data;
    struct ds3231_t *data;

    if (!mdev)
        return -ENODEV;

    data = container_of(mdev, struct ds3231_t, miscdev);
    file->private_data = data;

    return 0;
}
// ds3231 read
static ssize_t ds3231_misc_read(struct file *file, char __user *buf,
                                size_t count, loff_t *ppos) {
    struct ds3231_t *data = file->private_data;
    u8 time_buf[3];
    int ret;

    if (!data || !data->client)
        return -ENODEV;
    if (count == 0)
        return 0;
    if (count > 3)
        return -EINVAL;
    if (!data->updata_ready && (file->f_flags & O_NONBLOCK)) {
        dev_info(&data->client->dev, "Data not ready, returning 0");
        return -EAGAIN; // 数据未准备好，返回0表示EOF
    }

    ret = wait_event_interruptible(data->wq, data->updata_ready);
    if (ret) {
        return ret;
    }
    mutex_lock(&data->lock);
    time_buf[0] = data->time.second;
    time_buf[1] = data->time.minutes;
    time_buf[2] = data->time.hour;
    data->updata_ready = false;
    mutex_unlock(&data->lock);

    dev_dbg(&data->client->dev,
            "misc_read buf: sec=%u min=%u hour=%u count=%zu",
            time_buf[0], time_buf[1], time_buf[2], count);
    if (copy_to_user(buf, time_buf, count))
        return -EFAULT;
    return count;
}
// ds3231 write
static ssize_t ds3231_misc_write(struct file *file, const char __user *buf,
                                 size_t count, loff_t *ppos) {
    struct ds3231_t *data = file->private_data;
    u8 *time_buf;
    int ret;

    if (!data || !data->client)
        return -ENODEV;
    if (count == 0)
        return 0;
    if (count > 3)
        return -EINVAL;

    time_buf = memdup_user(buf, count);

    if (IS_ERR(time_buf))
        return PTR_ERR(time_buf);
    mutex_lock(&data->lock);
    ret = ds3231_write_time(data->client, data, time_buf);
    mutex_unlock(&data->lock);

    kfree(time_buf);
    if (ret != 0) {
        dev_err(&data->client->dev, "Failed to write time: %d", ret);
        return ret;
    }
    return count;
}
// ds3231 poll
static unsigned int ds3231_misc_poll(struct file *file, poll_table *wait) {
    struct ds3231_t *data = file->private_data;
    unsigned int mask = 0;

    if (!data || !data->client)
        return -ENODEV;

    poll_wait(file, &data->wq, wait);

    mutex_lock(&data->lock);
    if (data->updata_ready) {
        mask |= POLLIN | POLLRDNORM; // 可读
    }
    mutex_unlock(&data->lock);

    return mask;
}

static const struct file_operations ds3231_misc_fops = {
    .owner = THIS_MODULE,
    .open = ds3231_misc_open,
    .read = ds3231_misc_read,
    .write = ds3231_misc_write,
    .poll = ds3231_misc_poll,
};

int ds3231_misc_register(struct ds3231_t *data) {
    int ret;
    if (!data || !data->client)
        return -EINVAL;
    data->miscdev.minor = MISC_DYNAMIC_MINOR;
    data->miscdev.name = DS3231_NAME;
    data->miscdev.fops = &ds3231_misc_fops;
    data->miscdev.parent = &data->client->dev;

    ret = misc_register(&data->miscdev);
    if (ret)
        dev_err(&data->client->dev, "Failed to register misc device: %d\n",
                ret);
    else
        dev_info(&data->client->dev,
                 "Misc device registered successfully, path = /dev/%s\n",
                 DS3231_NAME);
    return ret;
}

void ds3231_misc_unregister(struct ds3231_t *data) {
    if (data)
        misc_deregister(&data->miscdev);
}
