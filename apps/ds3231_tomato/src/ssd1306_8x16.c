#define FONT8x16_IMPLEMENTATION
#include "ssd1306_8x16.h"
#include "font8x16_ascii.h"
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define WIDTH 128
#define HEIGHT 64
#define FB_SIZE (WIDTH * HEIGHT / 8)
#define DEV_PATH "/dev/ssd1306"
void ssd1306_buffer_clear(uint8_t *file_buff) {
    memset(file_buff, 0x00, SSD1306_file_buff_SIZE);
}

void ssd1306_draw_buffer_char8x16(uint8_t *file_buff, int x, int page,
                                  char ch) {

    if (!(page >= 0 && page + 1 <= 7)) {
        printf("Page set invalid\n");
        return;
    }

    if (x + 7 >= SSD1306_W) {
        printf("x set invalid\n");
        return;
    }
    unsigned char uch = (unsigned char)ch;
    const uint8_t *g = font8x16[uch];

    int col;
    // 行扫描转列扫描
    for (col = 0; col < 8; col++) {
        uint8_t high_8bits_data = 0;
        uint8_t low_8bits_data = 0;
        // high_8bits_data
        for (int row = 0; row < 8; row++) {
            if (g[row] & (1u << (7 - col)))
                high_8bits_data |= (1u << row); // 填入lsb
        }
        // low_8bits_data
        for (int row = 0; row < 8; row++) {
            if (g[8 + row] & (1u << (7 - col)))
                low_8bits_data |= (1u << row);
        }

        file_buff[page * SSD1306_W + x + col] = high_8bits_data;
        file_buff[(page + 1) * SSD1306_W + x + col] = low_8bits_data;
    }
}
void ssd1306_draw_buffer_string8x16(uint8_t *file_buff, int x, int page,
                                    const char *s) {
    while (*s) {
        ssd1306_draw_buffer_char8x16(file_buff, x, page, *s++);
        x += 8;
        if (x > SSD1306_W - 8)
            break;
    }
}

void ssd1306_draw_buffer_image32x32(uint8_t *file_buff, int x, int page,
                                    const uint8_t *img) {
    if (!(page >= 0 && page + 3 <= 7)) {
        printf("Page set invalid\n");
        return;
    }

    if (x + 31 >= SSD1306_W) {
        printf("x set invalid\n");
        return;
    }

    /* img 格式：6字节头 + 128字节 page-major 数据
     * 每 32 字节一页（8行），共 4 页，直接按序拷贝 */
    const uint8_t *p = img + 6;

    for (int pg = 0; pg < 4; pg++) {
        for (int col = 0; col < 32; col++) {
            file_buff[(page + pg) * SSD1306_W + x + col] = p[pg * 32 + col];
        }
    }
}

static int write_all(int fd, const void *buf, size_t len) {
    const uint8_t *p = (const uint8_t *)buf;
    size_t left = len;

    while (left > 0) {
        ssize_t n = write(fd, p, left);
        // printf("write %zu bytes, n=%zd\n", left, n);
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

int send_buffer(int fd, uint8_t *buffer) {
    if (fd < 0) {
        perror("open");
        return -1;
    }
    if (write_all(fd, buffer, FB_SIZE) < 0) {
        perror("write_all");
        close(fd);
        return -1;
    }

    return 0;
}
