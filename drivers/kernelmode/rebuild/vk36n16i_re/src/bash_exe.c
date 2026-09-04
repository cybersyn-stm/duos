#include "bash_exe.h"
#include "linux/umh.h"
#include "stddef.h"
int bash_execute(const char *cmd) {
    char *argv[] = {"/bin/sh", "-c", (char *)cmd, NULL};

    char *envp[] = {"HOME=/", NULL};

    int ret;
    /*
     * UMH_WAIT_PROC: 等待脚本执行完成（会睡眠）
     * UMH_WAIT_EXEC: 等待 exec 成功返回（不等执行完）
     * UMH_NO_WAIT:   不等待
     */
    ret = call_usermodehelper(argv[0], argv, envp, UMH_WAIT_PROC);
    if (ret != 0) {
        printk(KERN_ERR "Failed to execute command: %s, error code: %d\n", cmd,
               ret);
    } else {
        printk(KERN_INFO "Command executed successfully: %s\n", cmd);
    }
    return ret;
}

int duo_pinmux_init(void) {
    int ret = 0;
    printk(KERN_INFO "Loading duo-pinmux kernel module\n");

    printk(KERN_INFO "Configuring B12 as IIC1_SCL\n");
    ret = bash_execute("duo-pinmux -w B12/IIC1_SCL");
    if (ret != 0) {
        printk(KERN_ERR "Failed to configure B12\n");
        return ret;
    }

    printk(KERN_INFO "Configuring B11 as IIC1_SDA\n");
    ret = bash_execute("duo-pinmux -w B11/IIC1_SDA");
    if (ret != 0) {
        printk(KERN_ERR "Failed to configure B11\n");
        return ret;
    }

    printk(KERN_INFO "duo-pinmux configuration completed successfully\n");
    return 0;
}
