#ifndef DS3231_F
#define DS3231_F

#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/wait.h>

#define DS3231_NAME "ds3231"
#define DS3231_Address 0x68

struct ds3231_time_t {
    uint8_t second;
    uint8_t minutes;
    uint8_t hour;
    uint8_t week;
    uint8_t mouth;
    uint8_t year;
};

struct ds3231_t {
    struct ds3231_time_t time;
    // miscdevice
    struct miscdevice miscdev;
    // i2c_clinet
    struct i2c_client *client;
    // mutex
    struct mutex lock;
    // interrupt
    int irq;
    // wait queue
    wait_queue_head_t wq;
    bool updata_ready;
};

int ds3231_read_time(struct i2c_client *client, struct ds3231_t *time);
int ds3231_write_time(struct i2c_client *client, struct ds3231_t *time,
                      u8 *data);
#endif // !DS3231_F
