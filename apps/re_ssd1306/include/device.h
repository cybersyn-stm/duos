#ifndef __INPUT_H__
#define __INPUT_H__
typedef struct device_information {
    int device_id;
    char *device_name;
} device_information;

typedef struct device_operation {
    int (*open_device)(const char *device_name);
    int (*close_device)(int device_fd);
    int (*read_device)(device_information *device);
} device_operation;
int open_vk36n16i(const char *device_name);
int close_vk36n16i(int fd);
int input_read_key(device_information *device);

int open_ssd1306(const char *device_name);
int close_ssd1306(int fd);
#endif // __INPUT_H__
