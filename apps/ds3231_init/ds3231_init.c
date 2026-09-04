#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

int main() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm t;
    localtime_r(&ts.tv_sec, &t);
    unsigned char time[3];
    // 等待到下一整秒开始，减小写入偏差（可选）
    if (ts.tv_nsec > 500000000) {
        usleep(1000000 - ts.tv_nsec / 1000);
        clock_gettime(CLOCK_REALTIME, &ts);
        localtime_r(&ts.tv_sec, &t);
    }

    int fd = open("/dev/ds3231", O_RDWR);
    if (fd < 0) {
        perror("open /dev/ds3231");
        return 1;
    }

    unsigned char buf[3] = {t.tm_hour, t.tm_min, t.tm_sec};
    if (write(fd, buf, 3) != 3) {
        perror("write");
        close(fd);
        return 1;
    }
    printf("RTC set to %02d:%02d:%02d\n", t.tm_hour, t.tm_min, t.tm_sec);

    if (read(fd, &time, 3) != 3) {
        perror("read /dev/ds3231");
        close(fd);
        return 1;
    }
    printf("RTC read back: %02d:%02d:%02d\n", time[2], time[1], time[0]);

    close(fd);
    return 0;
}
