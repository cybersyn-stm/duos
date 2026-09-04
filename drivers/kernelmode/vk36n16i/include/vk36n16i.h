#ifndef __VK36N16I_H
#define __VK36N16I_H

#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

#define VK36N16I_NAME "vk36n16i"

struct vk36n16i_data {
    struct i2c_client *client;
    struct mutex lock;

    /* 你原有字段：缓存 raw */
    u16 key_value;

    /* ===== 新增：轮询 + 阻塞用户接口 ===== */
    struct delayed_work poll_work;
    wait_queue_head_t wq;
    bool event_pending;

    u16 last_raw;    /* 最近一次轮询读到的 raw */
    u16 last_report; /* 最近一次对用户态“上报”的 raw，用于变化检测 */

    struct miscdevice miscdev; /* /dev/vk36n16i */
};

static int vk36n16i_read(struct i2c_client *client, uint16_t *data);

static int vk36n16i_decode_logic_key(uint16_t raw);
#endif /* __VK36N16I_H */
