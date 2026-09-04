#include "ssd1306_8x16.h"
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEV_PATH "/dev/ssd1306"
#define WIDTH 128
#define HEIGHT 64
#define FB_SIZE (WIDTH * HEIGHT / 8)

// write file
static int write_all(int fd, const void *buf, size_t len) {
    const uint8_t *p = (const uint8_t *)buf;
    size_t left = len;

    while (left > 0) {
        ssize_t n = write(fd, p, left);
        printf("write %zu bytes, n=%zd\n", left, n);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        p += (size_t)n;
        left -= (size_t)n;
    }
    return 0;
}

int send_buffer(uint8_t *buffer) {
    int fd = open(DEV_PATH, O_WRONLY);

    if (fd < 0) {
        perror("open");
        return -1;
    }
    if (write_all(fd, buffer, FB_SIZE) < 0) {
        perror("write_all");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

int main(int argc, char **argv) {

    uint8_t file_buffer[FB_SIZE];
    ssd1306_buffer_clear(file_buffer);

    int x = (int)strtol(argv[1], NULL, 10);
    int page = (int)strtol(argv[2], NULL, 10);

    if (argc == 1) {
        printf("Need enter\n");
        return -1;
    }

    else if (argc > 1) {
        ssd1306_draw_buffer_string8x16(file_buffer, x, page, argv[3]);
    }

    send_buffer(file_buffer);
    return 0;
}
