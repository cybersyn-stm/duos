#ifndef TM1650_H
#define TM1650_H

#include "linux/i2c.h"
#include "linux/miscdevice.h"

#define TM1650_ADDR_CMD 0x24
#define TM1650_ADDR_DIG1 0x34
#define TM1650_ADDR_DIG2 0x35
#define TM1650_ADDR_DIG3 0x36
#define TM1650_ADDR_DIG4 0x37
/* 7 段编码表 */
static const uint8_t segment_code[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D,
                                       0x7D, 0x07, 0x7F, 0x6F, 0x77, 0x7C,
                                       0x39, 0x5E, 0x79, 0x71};
struct tm1650_system {
    // 设备管理
    struct i2c_client *client;
    struct miscdevice mdev;
    // 指令和数据
    int command;
    int data;
    struct mutex lock;
};

int tm1650_write_byte(struct tm1650_system *sys, uint8_t cmd, uint8_t data);

#endif
