#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define DS3231_DEV_PATH "/dev/ds3231"

static int ds3231_device_open(int *fd) {
    *fd = open(DS3231_DEV_PATH, O_RDWR);
    if (*fd < 0) {
        perror("Failed to open DS3231 device");
        return -1;
    }
    printf("DS3231 device opened successfully with file descriptor: %d\n", *fd);
    return 0;
}

static int ds3231_device_close(int *fd) {
    if (close(*fd) < 0) {
        perror("Failed to close DS3231 device");
        return -1;
    }
    printf("DS3231 device closed successfully\n");
    return 0;
}

static int ds3231_device_read(int *fd, unsigned char *buffer, size_t size) {
    ssize_t bytes_read = read(*fd, buffer, size);
    if (bytes_read < 0) {
        perror("Failed to read from DS3231 device");
        return -1;
    }
    if ((size_t)bytes_read != size) {
        fprintf(stderr, "Short read: expected %zu, got %zd\n", size,
                bytes_read);
        return -1;
    }
    return 0;
}

static int time_to_seconds(const struct tm *tm) {
    return tm->tm_hour * 3600 + tm->tm_min * 60 + tm->tm_sec;
}

int main(int argc, char *argv[]) {
    int fd;
    unsigned char t[3];
    int iterations = 10;
    int interval_ms = 1000;
    int min_diff = 0;
    int max_diff = 0;
    long long sum_abs_diff = 0;

    if (argc > 1)
        iterations = atoi(argv[1]);
    if (argc > 2)
        interval_ms = atoi(argv[2]);
    if (iterations <= 0 || interval_ms <= 0) {
        fprintf(stderr, "Usage: %s [iterations] [interval_ms]\n", argv[0]);
        return 1;
    }

    if (ds3231_device_open(&fd) < 0)
        return 1;

    for (int i = 0; i < iterations; i++) {
        struct timespec ts;
        struct tm tm;
        int sys_sec;
        int ds_hour, ds_min, ds_sec;
        int diff;

        // 读取 RTC 时间：驱动返回顺序为 时、分、秒
        if (ds3231_device_read(&fd, t, 3) < 0)
            break;

        // 获取系统时间
        if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
            perror("clock_gettime");
            break;
        }
        localtime_r(&ts.tv_sec, &tm);

        sys_sec = time_to_seconds(&tm);

        // 驱动已将 BCD 转为二进制，t[0]=时, t[1]=分, t[2]=秒
        ds_hour = t[2];
        ds_min = t[1];
        ds_sec = t[0];

        int ds_sec_of_day = ds_hour * 3600 + ds_min * 60 + ds_sec;
        diff = ds_sec_of_day - sys_sec;

        // 处理跨日情况（差值在正负12小时内）
        if (diff > 43200)
            diff -= 86400;
        else if (diff < -43200)
            diff += 86400;

        if (i == 0) {
            min_diff = max_diff = diff;
        } else {
            if (diff < min_diff)
                min_diff = diff;
            if (diff > max_diff)
                max_diff = diff;
        }
        sum_abs_diff += llabs(diff);

        printf("raw bytes: %u %u %u\n", t[0], t[1], t[2]);
        printf("ds=%02u:%02u:%02u sys=%02d:%02d:%02d diff=%d sec\n", ds_hour,
               ds_min, ds_sec, tm.tm_hour, tm.tm_min, tm.tm_sec, diff);

        usleep(interval_ms * 1000);
    }

    if (iterations > 0) {
        double avg_abs = (double)sum_abs_diff / iterations;
        printf("diff summary: min=%d sec max=%d sec avg_abs=%.2f sec\n",
               min_diff, max_diff, avg_abs);
    }

    ds3231_device_close(&fd);
    return 0;
}
