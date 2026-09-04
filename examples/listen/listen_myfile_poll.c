#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t g_running = 1;

static void on_signal(int signo) {
    (void)signo;
    g_running = 0;
}

int main(int argc, char *argv[]) {
    const char *path = "/proc/myfile";
    long interval_ms = 1000;
    char buf[512];

    if (argc > 1) {
        path = argv[1];
    }
    if (argc > 2) {
        char *end = NULL;
        interval_ms = strtol(argv[2], &end, 10);
        if (end == argv[2] || *end != '\0' || interval_ms <= 0) {
            fprintf(stderr, "Invalid interval_ms: %s\n", argv[2]);
            return 1;
        }
    }

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    while (g_running) {
        int fd = open(path, O_RDONLY);
        if (fd < 0) {
            fprintf(stderr, "open(%s) failed: %s\n", path, strerror(errno));
        } else {
            ssize_t n = read(fd, buf, sizeof(buf) - 1);
            if (n < 0) {
                fprintf(stderr, "read(%s) failed: %s\n", path, strerror(errno));
            } else if (n > 0) {
                buf[n] = '\0';
                printf("read bytes: %zd\n", n);
                printf("%s", buf);
                fflush(stdout);
                close(fd);
                break;
            }
            close(fd);
        }

        struct timespec ts;
        ts.tv_sec = interval_ms / 1000;
        ts.tv_nsec = (interval_ms % 1000) * 1000000L;
        nanosleep(&ts, NULL);
    }

    return 0;
}
