#include "pomodoro.h"
#include "fcntl.h"
#include "stdio.h"
#include "sys/stat.h"

#define FIFO_PATH "/tmp/pomodoro"
int pomodoro_fifo_init(int fd) {
    mkfifo(FIFO_PATH, 0666);
    fd = open(FIFO_PATH, O_RDWR | O_NONBLOCK);
    if (fd < 0) {
        perror("open fifo");
        return -1;
    }
    return 1;
}
