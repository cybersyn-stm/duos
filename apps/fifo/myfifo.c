#include "fcntl.h"
#include "poll.h"
#include "stdio.h"
#include "string.h"
#include "sys/stat.h"
#include "unistd.h"

#define FIFO_PATH "/tmp/myfifo"

int main(void) {
    // 创建fifo
    mkfifo(FIFO_PATH, 0666);
    int fd = open(FIFO_PATH, O_RDWR | O_NONBLOCK);
    printf("[demo] FIFO ready. pid=%d\n", getpid());
    printf("[demo] echo 'xxx' > %s\n", FIFO_PATH);

    struct pollfd pfd = {.fd = fd, .events = POLLIN};
    char last[64] = {0}; // 记录字符串

    while (1) {
        poll(&pfd, 1, -1); // 阻塞

        char buf[64] = {0};
        int n = read(fd, buf, sizeof(buf) - 1);
        if (n <= 0)
            continue;

        if (strcmp(buf, "quit") == 0) {
            printf("[demo] quitting\n");
            break;
        }
        if (strcmp(buf, "repeat") == 0) {
            printf("[demo] %s\n", buf);
            continue;
        }

        strncpy(last, buf, sizeof(last) - 1);
        printf("[demo] %s\n", buf);
    }
    close(fd);
    unlink(FIFO_PATH);
    return 0;
}
