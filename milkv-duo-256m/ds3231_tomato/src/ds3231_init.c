#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define DS3231_DEV "/dev/ds3231"
#define POMO_FIFO_PATH "/tmp/pomodoro_fifo"

int ds3231_init(int fd) {
    struct timespec ts;
    struct tm t;

    /* 读取当前系统时间 */
    clock_gettime(CLOCK_REALTIME, &ts);
    localtime_r(&ts.tv_sec, &t);

    /* 预计算下一整秒的时间（在 sleep 之前算好，写的时候不再有任何计算开销） */
    t.tm_sec++;
    if (t.tm_sec >= 60) {
        t.tm_sec = 0;
        t.tm_min++;
        if (t.tm_min >= 60) {
            t.tm_min = 0;
            t.tm_hour++;
            if (t.tm_hour >= 24)
                t.tm_hour = 0;
        }
    }
    unsigned char buf[3] = {t.tm_hour, t.tm_min, t.tm_sec};

    /* 对齐到下一整秒边界：
     *  先用 usleep 睡到 ~100µs 之前（避免忙等浪费 CPU）
     *  再用 spin-wait 精确对齐到秒边界             */
    long ns_to_next_sec = 1000000000L - ts.tv_nsec;
    if (ns_to_next_sec > 150000) /* > 150µs → 值得睡 */
        usleep((unsigned int)(ns_to_next_sec / 1000) - 100);

    /* 自旋等待，直到 tv_sec 递增，即刚好跨越秒边界 */
    struct timespec ts_boundary;
    do {
        clock_gettime(CLOCK_REALTIME, &ts_boundary);
    } while (ts_boundary.tv_sec == ts.tv_sec);

    /* 现在紧贴秒边界 —— 直接写入预计算好的值，零额外开销 */
    if (write(fd, buf, 3) != 3) {
        perror("write");
        return 1;
    }
    printf("RTC set to %02d:%02d:%02d\n", t.tm_hour, t.tm_min, t.tm_sec);

    /* 回读校验 */
    unsigned char time[3];
    if (read(fd, &time, 3) != 3) {
        perror("read /dev/ds3231");
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

int main(void) {
    int mode = 0;           // 0: clock, 1: pomodoro
    int pomodoro_state = 0; // 0: idle, 1: running, 2: stopped
    // signal(SIGINT, handle_signal);
    // signal(SIGTERM, handle_signal);

    struct winsize w;
    int fd_ds3231 = open_device(DS3231_DEV);
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
    int pomodoro_time = 0;

    struct pollfd pfds[2] = {
        {.fd = fd_ds3231, .events = POLLIN},
        {.fd = fd_pomodoro, .events = POLLIN},
    };
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    printf("\033[2;%dr", 2);
    while (1) {
        int ret = poll(pfds, 2, -1);
        if (ret < 0) {
            if (errno == EINTR && g_quit)
                break;
            perror("poll");
            break;
        }

        if (pfds[1].revents & POLLIN) {
            char cmd[64] = {0};
            // 读取FIFO指令
            int n = read(fd_pomodoro, cmd, sizeof(cmd) - 1);
            // n大于0，且下一个字符为换行符或者回车符，替换为结束符
            while (n > 0 && (cmd[n - 1] == '\n' || cmd[n - 1] == '\r'))
                cmd[--n] = '\0';
            printf("Received command: %s\n", cmd);
            // strcmp == string compare
            if (strcmp(cmd, "pomodoro-start") == 0) {
                pomodoro_state = 1;
                pomodoro_time = 25 * 60; // 25 minutes
                mode = 1;
            } else if (strcmp(cmd, "pomodoro-stop") == 0) {
                pomodoro_state = 0;
                pomodoro_time = 0;
                mode = 0;
            } else if (strcmp(cmd, "clock") == 0) {
                mode = 0;
            } else if (strcmp(cmd, "pomodoro") == 0) {
                mode = 1;
            } else {
                printf("Unknown command: %s\n", cmd);
            }
        }
        if (pfds[0].revents & POLLIN) {
            if (read(fd_ds3231, time_buf, 3) != 3) {
                perror("read /dev/ds3231");
                break;
            }
            if (mode == 0) {
                printf("\033[s\033[H\033[KCurrent time: %02d:%02d:%02d\033[u",
                       time_buf[2], time_buf[1], time_buf[0]);
                fflush(stdout);
            }
            if (mode == 1 && pomodoro_state == 1) {
                pomodoro_time--;
                printf("\033[s\033[H\033[KPomodoro: %02d:%02d\033[u",
                       pomodoro_time / 60, pomodoro_time % 60);
                fflush(stdout);
            }
        }
    }

    close(fd_ds3231);
    close(fd_pomodoro);
    unlink(POMO_FIFO_PATH);
    printf("Exited cleanly\n");
    return 0;
}
