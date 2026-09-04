#include "device.h"
#include <fcntl.h> //open
#include <stdio.h>
#include <unistd.h> //close
int open_vk36n16i(const char *device_name) {
    int fd;
    fd = open(device_name, O_RDWR | O_NONBLOCK);
    if (fd < 0) {
        return -1;
    }

    return fd;
}

int close_vk36n16i(int fd) {
    if (close(fd) < 0) {
        perror("close key device fail");
        return -1;
    }
    return 0;
}

int input_read_key(device_information *device) {
    int fd = device->device_id;
    int key_value = 0;
    int rd;
    if (fd < 0) {
        perror("open key device fail");
        return -1;
    }
    rd = read(fd, &key_value, sizeof(int));
    if (rd == -1) {
        return -1;
    }
    return key_value;
}

int open_ssd1306(const char *device_name) {
    int fd;
    fd = open(device_name, O_RDWR);
    if (fd < 0) {
        return -1;
    }

    return fd;
}

int close_ssd1306(int fd) {
    if (close(fd) < 0) {
        perror("close ssd1306 device fail");
        return -1;
    }
    return 0;
}
