#include "nixietube.h"
#include <fcntl.h>
#include <gpiod.h>
#include <linux/gpio.h>
#include <poll.h>
#include <stdio.h>
#include <unistd.h>
int ds3231_init(int fd) {
    struct timespec ts;
    struct tm t;

    /* 读取当前系统时间 */
    clock_gettime(CLOCK_REALTIME, &ts);
    localtime_r(&ts.tv_sec, &t);

    /* 预计算下一整秒的时间（在 sleep 之前算好，写的时候不再有任何计算开销） */
    t.tm_sec++;
    if (t.tm_sec >= 60) {
        t.tm_sec = 0;
        t.tm_min++;
        if (t.tm_min >= 60) {
            t.tm_min = 0;
            t.tm_hour++;
            if (t.tm_hour >= 24)
                t.tm_hour = 0;
        }
    }
    unsigned char buf[3] = {t.tm_hour, t.tm_min, t.tm_sec};

    /* 对齐到下一整秒边界：
     *  先用 usleep 睡到 ~100µs 之前（避免忙等浪费 CPU）
     *  再用 spin-wait 精确对齐到秒边界             */
    long ns_to_next_sec = 1000000000L - ts.tv_nsec;
    if (ns_to_next_sec > 150000) /* > 150µs → 值得睡 */
        usleep((unsigned int)(ns_to_next_sec / 1000) - 100);

    /* 自旋等待，直到 tv_sec 递增，即刚好跨越秒边界 */
    struct timespec ts_boundary;
    do {
        clock_gettime(CLOCK_REALTIME, &ts_boundary);
    } while (ts_boundary.tv_sec == ts.tv_sec);

    /* 现在紧贴秒边界 —— 直接写入预计算好的值，零额外开销 */
    if (write(fd, buf, 3) != 3) {
        perror("write");
        return 1;
    }
    printf("RTC set to %02d:%02d:%02d\n", t.tm_hour, t.tm_min, t.tm_sec);

    /* 回读校验 */
    unsigned char time[3];
    if (read(fd, &time, 3) != 3) {
        perror("read /dev/ds3231");
        return 1;
    }
    printf("RTC read back: %02d:%02d:%02d\n", time[2], time[1], time[0]);

    return 0;
}

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
    printf("Bit:");
    for (int i = 7; i >= 0; i--) { // MSB 先
        gpiod_line_set_value(ser, (data >> i) & 1);
        printf("%d", (data >> i) & 1);
        gpiod_line_set_value(srclk, 1); // 上升沿移入
        gpiod_line_set_value(srclk, 0);
    }
    printf("\n");
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

unsigned char bcd_to_bin(unsigned char bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

int main() {
    printf("NIXIETUBE\n");
    int fd_ds3231 = open_device("/dev/ds3231");
    unsigned char time_buf[3];
    struct pollfd pfds[2] = {{.fd = fd_ds3231, .events = POLLIN}};
    if (gpio_init() < 0) {
        printf("Failed to init GPIO\n");
        close_device(fd_ds3231);
        return -1;
    }
    ds3231_init(fd_ds3231);
    shift_out(0x01);
    while (1) {
        poll(pfds, 1, -1);
        if (pfds[0].revents & POLLIN) {
            read(fd_ds3231, time_buf, 3);
            printf("Current time: %d:%d:%d\n", time_buf[2], time_buf[1],
                   time_buf[0]);
        }
    }
    close_device(fd_ds3231);
    gpio_cleanup();
    return 0;
}
