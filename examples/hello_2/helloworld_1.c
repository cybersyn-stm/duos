#include "linux/module.h"
#include "linux/printk.h"
#include "linux/proc_fs.h"
#include "linux/string.h"
#include "linux/types.h"
#include "linux/uaccess.h"
#define File_Name "myfile"

static char pro_data[256] = "Hello, World!";

static ssize_t write_file(struct file *file, const char __user *buf,
                          size_t count, loff_t *ppos) {
    size_t to_copy = count;

    if (to_copy >= sizeof(pro_data)) {
        to_copy = sizeof(pro_data) - 1; // Leave space for null terminator
    }

    if (copy_from_user(pro_data, buf, to_copy)) {
        return -EFAULT; // Return error if copy fails
    }

    pro_data[to_copy] = '\0'; // Null-terminate the string

    *ppos = 0;

    return to_copy; // Return the number of bytes written
}

static ssize_t read_file(struct file *file, char __user *buf, size_t count,
                         loff_t *ppos) {
    size_t len = strlen(pro_data);

    if (*ppos >= len) {
        return 0;
    }

    if (copy_to_user(buf, pro_data, strlen(pro_data) + 1)) {
        return -EFAULT; // Return error if copy fails
    }

    *ppos += count;
    return len; // Return the number of bytes read
}

static int open_file(struct inode *inode, struct file *file) {
    printk(KERN_INFO "File opened.\n");
    printk(KERN_INFO "Inode number: %lu\n", inode->i_ino);
    printk(KERN_INFO "File mode: %o\n", inode->i_mode);
    printk(KERN_INFO "File size: %lld bytes\n", inode->i_size);
    printk(KERN_INFO "File permissions: %o\n", inode->i_mode & 0777);
    printk(KERN_INFO "File owner UID: %u\n", inode->i_uid.val);
    printk(KERN_INFO "File group GID: %u\n", inode->i_gid.val);
    return 0;
}

static int open_exit(struct inode *inode, struct file *file) {
    printk(KERN_INFO "File closed.\n");
    return 0;
}
static const struct proc_ops proc_file_ops = {
    .proc_open = open_file,
    .proc_release = open_exit,
    .proc_read = read_file,
    .proc_write = write_file,
};

static int __init hello_init(void) {
    proc_create(File_Name, 0666, NULL, &proc_file_ops);
    printk(KERN_INFO "Hello, World! Module loaded.\n");
    return 0;
}

static void __exit hello_exit(void) {
    remove_proc_entry(File_Name, NULL);
    printk(KERN_INFO "Hello, World! Module unloaded.\n");
}

MODULE_LICENSE("GPL");
module_init(hello_init);
module_exit(hello_exit);
