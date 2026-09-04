#include "tm1650_misc.h"
#include "linux/fs.h"
#include "linux/kernel.h"
static int tm1650_open(struct inode *inode, struct file *file) {
    struct miscdevice *mdev = file->private_data;
    struct tm1650_system *sys = container_of(mdev, struct tm1650_system, mdev);
    file->private_data = sys;
    return 0;
}

static ssize_t tm1650_write(struct file *file, const char __user *buf,
                            size_t count, loff_t *ppos) {
    struct tm1650_system *sys = file->private_data;
    char kbuf[2];
    if (count != 2) {
        return -EINVAL;
    }
    if (copy_from_user(kbuf, buf, 2)) {
        return -EFAULT;
    }
    mutex_lock(&sys->lock);
    sys->command = kbuf[0];
    sys->data = kbuf[1];
    tm1650_write_byte(sys, sys->command, sys->data);
    mutex_unlock(&sys->lock);
    return count;
}

static const struct file_operations tm1650_fops = {
    .owner = THIS_MODULE,
    .open = tm1650_open,
    .write = tm1650_write,
};

int tm1650_misc_init(struct tm1650_system *sys) {
    int ret;
    sys->mdev.minor = MISC_DYNAMIC_MINOR;
    sys->mdev.name = "tm1650";
    sys->mdev.fops = &tm1650_fops;
    mutex_init(&sys->lock);
    ret = misc_register(&sys->mdev);
    if (ret) {
        dev_err(&sys->client->dev, "Failed to register misc device: %d\n", ret);
        return ret;
    }
    dev_info(&sys->client->dev, "TM1650 misc device registered\n");
    return 0;
}

int tm1650_misc_exit(struct tm1650_system *sys) {
    misc_deregister(&sys->mdev);
    return 0;
}
