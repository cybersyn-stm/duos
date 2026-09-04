#ifndef __SSD1306_H_
#define __SSD1306_H_
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

typedef uint8_t buff_type;

int send_buffer(uint8_t *buffer, int fd);
int open_ssd1306();
int close_ssd1306(int fd);

#endif // !#ifndef __SSD1306_H_
