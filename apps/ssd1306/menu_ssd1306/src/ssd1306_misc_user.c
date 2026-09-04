#include "ssd1306.h"
#include "ssd1306_8x16.h"

const char *example_strings[8] = {
    "Hello, World!", "SSD1306 OLED", "Display Test", "Line 4 Text",
    "Line 5 Text",   "Line 6 Text",  "Line 7 Text",  "Line 8 Text"};

// write file
static int write_all(int fd, const void *buf, size_t len) {
    const uint8_t *p = (const uint8_t *)buf;
    size_t left = len;

    while (left > 0) {
        ssize_t n = write(fd, p, left);
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
