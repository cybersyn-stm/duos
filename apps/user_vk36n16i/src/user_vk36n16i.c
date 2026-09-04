#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

int main(void) {
    const char *path = "/dev/vk36n16i";
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    int logic;
    ssize_t n = read(fd, &logic, sizeof(logic));
    if (n < 0) {
        perror("read");
        close(fd);
        return 1;
    }
    if (n != sizeof(logic)) {
        fprintf(stderr, "short read: %zd\n", n);
        close(fd);
        return 1;
    }

    printf("logic = %d\n", logic);
    close(fd);
    return 0;
}
