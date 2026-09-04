#ifndef VK36N16I_H
#define VK36N16I_H

#include "bash_exe.h"
#include "linux/mutex.h"
#include "linux/poll.h"
#include "linux/types.h"
#include "linux/workqueue.h"
#include <linux/i2c.h>
#include <linux/kernel.h> //container_of
#include <linux/miscdevice.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/mutex.h>
#define VK36N16I_NAME "vk36n16i"

struct vk36n16i_data {
    struct i2c_client *client; // i2c客户端结构体
    struct mutex lock;         // 互斥锁

    u16 key_value; // 键值

    struct delayed_work poll_work;
    wait_queue_head_t wait_queue; // 等待队列列表头结构体
    bool event_pending;           // 事件待处理标志

    u16 last_key_value; // 上一次的键值
    u16 last_report;    // 上一次上报的键值

    struct miscdevice miscdev; // 杂项设备结构体
};

#endif
