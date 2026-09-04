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
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

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
    char time_pomodoro[16];
    int pomodoro_time = 0;
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

        /* FIFO 命令处理 */
        if (pfds[1].revents & POLLIN) {
            char cmd[64] = {0};
            int n = read(fd_pomodoro, cmd, sizeof(cmd) - 1);
            if (n > 0) {
                cmd[n] = '\0';
                while (n > 0 && (cmd[n - 1] == '\n' || cmd[n - 1] == '\r'))
                    cmd[--n] = '\0';
                printf("Received command: %s\n", cmd);
                if (strcmp(cmd, "pomodoro-start") == 0) {
                    pomodoro_time = 1500;
                    pomodoro_state = 1;
                    mode = 1;
                } else if (strcmp(cmd, "pomodoro-stop") == 0) {
                    pomodoro_time = 0;
                    pomodoro_state = 0;
                    mode = 0;
                } else if (strcmp(cmd, "clock") == 0) {
                    mode = 0;
                } else if (strcmp(cmd, "pomodoro") == 0) {
                    mode = 1;
                }
            }
        }

        /* RTC 秒中断 */
        if (pfds[0].revents & POLLIN) {
            if (read(fd_ds3231, time_buf, 3) != 3) {
                perror("read /dev/ds3231");
                break;
            }

            if (mode == 0) {
                /* 时钟模式 */
                snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d",
                         time_buf[2] + 8, time_buf[1], time_buf[0]);

                ssd1306_draw_buffer_string8x16(buf, mid_str(time_str), 3,
                                               time_str);
                printf("Current time: %s\n", time_str);

            } else if (mode == 1 && pomodoro_state == 1) {
                /* 番茄钟模式：倒计时 */
                pomodoro_time--; // 每秒减 1
                if (pomodoro_time > 0) {
                    snprintf(time_pomodoro, sizeof(time_pomodoro), "%02d:%02d",
                             pomodoro_time / 60, pomodoro_time % 60);
                    /*ssd1306_draw_buffer_image32x32(
                        buf, mid_str(gImage_tomato) - 6, 0, gImage_tomato);*/
                    ssd1306_draw_buffer_string8x16(buf, 0, 3,
                                                   "pomodoro_clock:");
                    ssd1306_draw_buffer_string8x16(buf, mid_str(time_pomodoro),
                                                   5, time_pomodoro);
                } else {
                    /* 归零 → 显示感叹号 */
                    pomodoro_state = 2; // 标记已结束
                    ssd1306_draw_buffer_string8x16(buf, (SSD1306_W - 8) / 2, 3,
                                                   "!");
                }
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
