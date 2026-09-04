#include "duo_pinmux.h"
#include "linux/kern_levels.h"
#include "linux/printk.h"
#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/slab.h>   // kmalloc
#include <linux/string.h> // strcat
char *combine_strings(char *array[], const char *delimiter) {
    size_t total_len = 0;
    char *result = NULL;
    size_t i;

    // 计算拼接后的字符串总长度
    for (i = 0; array[i] != NULL; i++) {
        total_len += strlen(array[i]) + strlen(delimiter); // 每个字符串+分隔符
    }

    // 为目标字符串分配内存
    result = kmalloc(total_len + 1, GFP_KERNEL); // 加1是为NULL终结符
    if (!result) {
        printk(KERN_ERR "Failed to allocate memory for combined string\n");
        return NULL;
    }

    // 初始化为空字符串
    result[0] = '\0';

    // 拼接字符串
    for (i = 0; array[i] != NULL; i++) {
        strcat(result, array[i]); // 添加当前数组元素
        if (array[i + 1] != NULL) {
            strcat(result, delimiter); // 添加分隔符（最后一个元素后不加分隔符）
        }
    }

    return result;
}

int execute_command(const char *cmd) {
    char *argv[] = {"/bin/sh", "-c", (char *)cmd, NULL};
    char *envp[] = {"HOME=/", NULL};

    int ret = call_usermodehelper(argv[0], argv, envp, UMH_WAIT_PROC);
    char *argv_print = combine_strings(argv, " ");
    printk(KERN_INFO "argv = %s, envp = %s\n", argv_print, *envp);
    if (ret != 0) {
        printk(KERN_ERR "Failed to execute:  %s (ret=%d)\n", cmd, ret);
        return ret;
    }

    printk(KERN_INFO "Successfully executed: %s\n", cmd);
    return 0;
}
