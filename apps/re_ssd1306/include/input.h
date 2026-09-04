#ifndef __INPUT_H__
#define __INPUT_H__

typedef struct InputOperation {
    int (*open_device)(int fd);
    int (*close_device)(int fd);
    int (*read_input)(int fd, char *buffer, int size);

} InputOperation;

typedef struct InputDevice {
    int fd;
    InputOperation ops;
} InputDevice;

int close_device(int fd);
int open_device(const char *device_path);
#endif
