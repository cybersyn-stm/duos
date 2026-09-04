#ifndef VK36N16I_H
#define VK36N16I_H

#include "linux/i2c.h"
#include "linux/miscdevice.h"
#include "linux/workqueue.h"

#define VK36N16I_NAME "vk36n16i"
extern unsigned int poll_ms;
struct vk36n16i_data {
    // 设备数据
    u16 raw;
    // 设备结构
    struct i2c_client *client;
    struct miscdevice mdev;
    struct mutex lock;
    // 轮询
    struct delayed_work poll_work;
    wait_queue_head_t poll_wq;
    bool event_pending;

    u16 last_report;
};

#endif
