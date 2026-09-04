#define _GNU_SOURCE
#include "kmod_utils.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

int kmod_is_loaded(const char *module_name) {
    char sys_path[256];
    struct stat st;

    if (!module_name || !*module_name)
        return -1;

    snprintf(sys_path, sizeof(sys_path), "/root/%s", module_name);
    if (stat(sys_path, &st) == 0)
        return 1;

    if (errno == ENOENT)
        return 0;

    return -1;
}

int kmod_insmod_file(const char *ko_path, const char *params) {
    int fd;
    int rc;

    if (!ko_path || !*ko_path)
        return -1;

    fd = open(ko_path, O_RDONLY);
    if (fd < 0)
        return -1;

    rc = syscall(SYS_finit_module, fd, params ? params : "", 0);
    if (rc == 0) {
        close(fd);
        return 0;
    }

    /* 如果 finit_module 不支持，fallback：读入内存再 init_module */
    if (errno == ENOSYS) {
        struct stat st;
        if (fstat(fd, &st) < 0) {
            close(fd);
            return -1;
        }

        /* 注意：精简系统可能没有 malloc；如果没有，就只能依赖 finit_module 或
         * insmod 命令 */
        void *buf = sbrk(0);
        (void)buf;
        close(fd);
        errno = ENOTSUP;
        return -1;
    }

    /* EEXIST：模块已加载也算成功（避免重复加载失败） */
    if (errno == EEXIST) {
        close(fd);
        return 0;
    }

    close(fd);
    return -1;
}

int wait_devnode(const char *path, int timeout_ms) {
    int waited = 0;
    struct stat st;

    if (!path)
        return -1;

    while (waited < timeout_ms) {
        if (stat(path, &st) == 0)
            return 0;
        usleep(10 * 1000);
        waited += 10;
    }
    return -1;
}
