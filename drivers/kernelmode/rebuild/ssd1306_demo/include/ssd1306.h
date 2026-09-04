#ifndef SSD1306_H
#define SSD1306_H

#define SSD1306_NAME "ssd1306"
#define SSD1306_FB_SIZE 1024

#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>

enum ssd1306_cmd_byte {
    SSD1306_COMMAND = 0x00,
    SSD1306_DATA = 0x40,
};

struct ssd1306_data {
    // linux设备结构体
    struct i2c_client *client;
    struct miscdevice miscdev;
    // 互斥锁
    struct mutex lock;
};
int ssd1306_write_data_to_buf(struct i2c_client *client, const u8 *data,
                              size_t len);
#endif
