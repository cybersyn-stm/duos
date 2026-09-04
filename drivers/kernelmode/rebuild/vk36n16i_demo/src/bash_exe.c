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
