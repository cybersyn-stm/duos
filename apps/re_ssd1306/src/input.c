#include "input.h"
#include <fcntl.h>
#include <linux/unistd.h>
#include <stdio.h>
#include <unistd.h>

int open_device(const char *device_path) {
    int fd;
    if ((fd = open(device_path, O_RDONLY | O_NONBLOCK)) < 0) {
        perror("Failed to open input device");
        return -1;
    }
    return fd;
}

int close_device(int fd) {
    if (close(fd) < 0) {
        perror("Failed to close input device");
        return -1;
    }
    return 0;
}
