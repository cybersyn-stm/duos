#include "linux/init.h"
#include "linux/module.h"
#include "linux/printk.h"
#include "linux/proc_fs.h"
#include "linux/types.h"
#include "linux/uaccess.h"
#include <linux/errno.h>
#include <linux/string.h>
#define PROC_NAME "myfile"

static ssize_t my_read(struct file *file, char __user *buf, size_t count,
                       loff_t *ppos) {
    // 实现你的 read 逻辑
    char *data = "Hello, World!\n";
    if (*ppos > 0) {
        return 0; // EOF
    }
    copy_to_user(buf, data, strlen(data));
    *ppos = strlen(data);
    printk(KERN_INFO "Reading data: %s\n", data);
    return strlen(data);
}

static ssize_t my_write(struct file *file, const char __user *buf, size_t count,
                        loff_t *ppos) {
    char kbuf[256];
    size_t to_copy = count;

    if (to_copy >= sizeof(kbuf)) {
        to_copy = sizeof(kbuf) - 1;
    }

    if (copy_from_user(kbuf, buf, to_copy)) {
        return -EFAULT;
    }

    kbuf[to_copy] = '\0';
    printk(KERN_INFO "Received data: %s\n", kbuf);
    *ppos += count;
    return count;
}
static int my_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "File opened\n");
    return 0;
};
static const struct proc_ops proc_fops = {
    .proc_open = my_open,
    .proc_read = my_read,
    .proc_write = my_write,
};

static int __init my_init(void) {
    proc_create(PROC_NAME, 0700, NULL, &proc_fops);
    return 0;
}

static void __exit my_exit(void) { remove_proc_entry(PROC_NAME, NULL); }

module_init(my_init);
module_exit(my_exit);
MODULE_LICENSE("GPL");
