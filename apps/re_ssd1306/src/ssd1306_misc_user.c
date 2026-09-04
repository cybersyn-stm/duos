#include "ssd1306.h"
#include "ssd1306_8x16.h"
#include <stdint.h>

/* write file */
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

/*
 * Send framebuffer to SSD1306.
 * Note:
 *   - fd must be a valid, already-open file descriptor for /dev/ssd1306
 *   - This function does NOT close(fd). Caller owns fd lifetime.
 */
int send_buffer(uint8_t *buffer, int fd) {
    if (fd < 0) {
        errno = EBADF;
        perror("send_buffer: invalid fd");
        return -1;
    }

    if (write_all(fd, buffer, FB_SIZE) < 0) {
        perror("send_buffer: write_all");
        return -1;
    }

    return 0;
}
