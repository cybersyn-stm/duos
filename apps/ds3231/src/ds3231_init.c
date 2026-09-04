#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pomodoro.h>
#include <signal.h>
#include <ssd1306_8x16.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define DS3231_DEV "/dev/ds3231"
#define SSD1306_DEV "/dev/ssd1306"

int ds3231_init(int fd) {
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

int mid_str(char *str) {
    int lenght = strlen(str) * 8;
    return (128 - lenght) / 2;
}

static volatile int g_quit = 0;

static void handle_signal(int sig) {
    (void)sig;
    g_quit = 1;
}

int main(int argc, char *argv[]) {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    int debug = 0;
    if (argc > 1 && strcmp(argv[1], "debug") == 0)
        debug = 1;

    int fd_ds3231 = open_device(DS3231_DEV);
    int fd_ssd1306 = open_device(SSD1306_DEV);
    ds3231_init(fd_ds3231);

    /* 创建 FIFO 命令管道 */
    mkfifo(POMO_FIFO_PATH, 0666);
    int fd_pomodoro = open(POMO_FIFO_PATH, O_RDWR | O_NONBLOCK);
    if (fd_pomodoro < 0) {
        perror("open FIFO");
        fd_pomodoro = -1;
    } else {
        printf("FIFO opened: %s (fd=%d)\n", POMO_FIFO_PATH, fd_pomodoro);
    }

    unsigned char time_buf[3];
    char time_str[16];
    uint8_t buf[1024];

    struct pollfd pfds[2] = {
        {.fd = fd_ds3231, .events = POLLIN},
        {.fd = fd_pomodoro, .events = POLLIN},
    };

    while (!g_quit) {
        int ret = poll(pfds, 2, -1);
        if (ret < 0) {
            if (errno == EINTR && g_quit)
                break;
            perror("poll");
            break;
        }

        ssd1306_buffer_clear(buf);

        /* 只有 RTC 触发时才读时间并绘制 */
        if (pfds[0].revents & POLLIN) {
            if (read(fd_ds3231, &time_buf, 3) != 3) {
                printf("Failed to read time from ds3231\n");
                break;
            }
            snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d",
                     time_buf[2] + 8, time_buf[1], time_buf[0]);
            ssd1306_draw_buffer_string8x16(buf, mid_str(time_str), 3, time_str);
            if (debug)
                printf("RTC: %s\n", time_str);
        }

        /* 只有 FIFO 有命令时才处理 */
        if (pfds[1].revents & POLLIN) {
            char cmd_buf[64] = {0};
            int n = read(fd_pomodoro, cmd_buf, sizeof(cmd_buf) - 1);
            if (n > 0) {
                cmd_buf[n] = '\0';
                while (n > 0 &&
                       (cmd_buf[n - 1] == '\n' || cmd_buf[n - 1] == '\r'))
                    cmd_buf[--n] = '\0';
                printf("Received command: %s\n", cmd_buf);
                if (strcmp(cmd_buf, "start") == 0)
                    ssd1306_draw_buffer_string8x16(buf, mid_str("POMO START"),
                                                   3, "POMO START");
                else if (strcmp(cmd_buf, "stop") == 0)
                    ssd1306_draw_buffer_string8x16(buf, mid_str("POMO STOP"), 3,
                                                   "POMO STOP");
            }
        }

        send_buffer(fd_ssd1306, buf);
    }

    close(fd_ds3231);
    close(fd_ssd1306);
    close(fd_pomodoro);
    unlink(POMO_FIFO_PATH);
    printf("Exited cleanly\n");
    return 0;
}
