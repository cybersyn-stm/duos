#include "linux/module.h"
#include "linux/printk.h"
#include "linux/proc_fs.h"
#include "linux/string.h"
#include "linux/types.h"
#include "linux/uaccess.h"
#define File_Name "myfile"

static char proc_data[256] = "Hello, World!\n";
static size_t proc_data_len = 14;
// file read
static ssize_t my_read(struct file *file, char __user *buf, size_t count,
                       loff_t *ppos) {
    size_t to_copy;

    if (*ppos >= proc_data_len) {
        return 0;
    }

    to_copy = proc_data_len - *ppos;
    if (to_copy > count) {
        to_copy = count;
    }

    if (copy_to_user(buf, proc_data + *ppos, to_copy)) {
        return -EFAULT;
    }

    *ppos += to_copy;
    printk(KERN_INFO "Read bytes: %zu\n", to_copy);
    return to_copy;
}
// file write
static ssize_t my_write(struct file *file, const char __user *buf, size_t count,
                        loff_t *ppos) {
    size_t to_copy = count;

    if (to_copy >= sizeof(proc_data)) {
        to_copy = sizeof(proc_data) - 1;
    }

    if (copy_from_user(proc_data, buf, to_copy)) {
        return -EFAULT;
    }

    proc_data[to_copy] = '\0';
    proc_data_len = to_copy;

    printk(KERN_INFO "Received data: %s\n", proc_data);
    *ppos = 0;
    return to_copy;
}
// file open
static int my_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "File opened\n");
    return 0;
}
// file file_operations
static const struct proc_ops proc_file_operations = {
    .proc_open = my_open,
    .proc_read = my_read,
    .proc_write = my_write,
};
// file init
static int __init my_init(void) {
    printk("=====================Init myfile=====================\n");
    proc_create(File_Name, 0700, NULL, &proc_file_operations);
    return 0;
}
// file exit
static void __exit my_exit(void) {
    printk("=====================Exit myfile=====================\n");
    remove_proc_entry(File_Name, NULL);
}
// module init and exit
module_init(my_init);
module_exit(my_exit);
MODULE_LICENSE("GPL");
