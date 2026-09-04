#include "vk36n16i_misc.h"
#include "linux/fs.h"
#include "linux/kernel.h"
#include "linux/miscdevice.h"
#include "linux/mutex.h"
#include "linux/poll.h"
#include "linux/wait.h"
#include "vk36n16i.h"
#include "vk36n16i_i2c.h"
static const uint8_t key_value_table[16] = {1, 4, 7, 15, 2,  5,  8,  0,
                                            3, 6, 9, 12, 10, 13, 11, 14};

static int vk36n16i_decode_logic_key(uint16_t raw) {
    int bit;

    if (raw == 0)
        return -1;

    bit = __ffs(raw); /* 0..15 */
    if (bit < 0 || bit >= 16)
        return -1;

    return key_value_table[bit];
}

static int vk36n16i_misc_open(struct inode *inode, struct file *file) {
    struct miscdevice *mdev = file->private_data;
    struct vk36n16i_data *data;
    if (mdev == NULL) {
        return -ENODEV;
    }
    data = container_of(mdev, struct vk36n16i_data, mdev);
    file->private_data = data;

    return 0;
}

static ssize_t vk36n16i_misc_read(struct file *file, char __user *buf,
                                  size_t count, loff_t *ppos) {
    struct vk36n16i_data *data = file->private_data;
    int ret;
    int raw;
    int key_value;

    if (count < sizeof(raw))
        return -EINVAL;
    if (file->f_flags & O_NONBLOCK) {
        if (!data->event_pending)
            return -EAGAIN;
    } else {
        ret = wait_event_interruptible(data->poll_wq, data->event_pending);
        if (ret)
            return -EAGAIN;
    }

    mutex_lock(&data->lock);
    data->event_pending = false;
    raw = data->raw;
    mutex_unlock(&data->lock);

    key_value = vk36n16i_decode_logic_key(raw);
    if (copy_to_user(buf, &key_value, sizeof(key_value)))
        return -EFAULT;
    return sizeof(key_value);
}

static void vk36n16i_poll_work(struct work_struct *work) {
    struct vk36n16i_data *data =
        container_of(work, struct vk36n16i_data, poll_work.work);

    u16 raw;
    int ret;

    ret = vk36n16i_read_key(data, &raw);

    if (!ret) {
        data->raw = raw;
        if (data->last_report != raw) {
            data->last_report = raw;
            data->event_pending = true;
        }
    }

    if (data->event_pending) {
        wake_up_interruptible(&data->poll_wq);
    }
    // 继续轮询
    schedule_delayed_work(&data->poll_work, msecs_to_jiffies(poll_ms));
}

static __poll_t vk36n16i_chr_poll(struct file *file, poll_table *wait) {
    struct vk36n16i_data *data = file->private_data;
    __poll_t mask = 0;

    poll_wait(file, &data->poll_wq, wait);

    if (READ_ONCE(data->event_pending))
        mask |= POLLIN | POLLRDNORM;

    return mask;
}

static const struct file_operations vk36n16i_fops = {
    .owner = THIS_MODULE,
    .open = vk36n16i_misc_open,
    .read = vk36n16i_misc_read,
    .poll = vk36n16i_chr_poll,
};

int vk36n16i_misc_init(struct i2c_client *client) {
    struct vk36n16i_data *data = i2c_get_clientdata(client);
    int ret;

    data->raw = 0;
    data->last_report = 0;
    data->event_pending = false;
    // 设备初始化
    data->mdev.minor = MISC_DYNAMIC_MINOR;
    data->mdev.name = VK36N16I_NAME;
    data->mdev.fops = &vk36n16i_fops;
    data->mdev.parent = &client->dev;

    init_waitqueue_head(&data->poll_wq);

    ret = misc_register(&data->mdev);
    if (ret) {
        dev_err(&client->dev, "Failed to register misc device\n");
        return ret;
    }
    INIT_DELAYED_WORK(&data->poll_work, vk36n16i_poll_work);
    schedule_delayed_work(&data->poll_work, msecs_to_jiffies(poll_ms));

    dev_info(&client->dev, "Misc device registered successfully\n");
    dev_info(&client->dev, "Polling every %u ms\n", poll_ms);
    return 0;
}

void vk36n16i_misc_exit(struct i2c_client *client) {
    struct vk36n16i_data *data = i2c_get_clientdata(client);

    cancel_delayed_work_sync(&data->poll_work);
    misc_deregister(&data->mdev);
    dev_info(&client->dev, "Misc device deregistered\n");
}
