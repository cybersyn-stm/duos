#include "nixietube.h"
#include <fcntl.h>
#include <gpiod.h>
#include <linux/gpio.h>
#include <poll.h>
#include <stdio.h>
#include <unistd.h>

int open_device(const char *dev) {
    int fd;
    if ((fd = open(dev, O_RDWR)) < 0) {
        printf("Failed to open %s\n", dev);
        return -1;
    }
    printf("Open device successfully: %s\n", dev);
    return fd;
}

int close_device(int fd) {
    if (close(fd) < 0) {
        perror("close");
        return -1;
    }
    printf("Close device successfully\n");
    return 0;
}

int gpio_init(void) {
    chip = gpiod_chip_open(CHIP);
    if (!chip) {
        return -1;
    }

    ser = gpiod_chip_get_line(chip, SER);
    rclk = gpiod_chip_get_line(chip, RCLK);
    srclk = gpiod_chip_get_line(chip, SRCLK);
    srclk_clean = gpiod_chip_get_line(chip, SRCLK_CLEAN);

    gpiod_line_request_output(ser, "74hc595", 0);
    gpiod_line_request_output(rclk, "74hc595", 0);
    gpiod_line_request_output(srclk, "74hc595", 0);
    gpiod_line_request_output(srclk_clean, "74hc595", 0);

    return 0;
}

void shift_out(unsigned char data) {
    for (int i = 7; i >= 0; i--) { // MSB 先
        gpiod_line_set_value(ser, (data >> i) & 1);
        gpiod_line_set_value(srclk, 1); // 上升沿移入
        gpiod_line_set_value(srclk, 0);
    }
    gpiod_line_set_value(rclk, 1); // 锁存
    gpiod_line_set_value(rclk, 0);
}

void gpio_cleanup(void) {
    gpiod_line_release(ser);
    gpiod_line_release(srclk);
    gpiod_line_release(rclk);
    gpiod_chip_close(chip);
}

int write_data_74HC595(int num) {
    if (num < 0 || num > 9) {
        printf("Invalid number: %d\n", num);
        return -1;
    }
    for (int i = 0; i < 8; i++) {
        if (num >> 1 & 0x01) {
        }
    }
    return 0;
}

int main() {
    printf("NIXIETUBE\n");
    int fd_ds3231 = open_device("/dev/ds3231");
    unsigned char time_buf[3];
    struct pollfd pfds[2] = {{.fd = fd_ds3231, .events = POLLIN}};
    while (1) {
        poll(pfds, 1, -1);
        read(fd_ds3231, time_buf, 3);
        if (pfds[0].revents & POLLIN) {
            printf("Current time: %02x:%02x:%02x\n", time_buf[0] + 8,
                   time_buf[1], time_buf[2]);
        }
    }
    close_device(fd_ds3231);
    return 0;
}
