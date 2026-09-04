#include "vk36n16i_i2c.h"
#include "linux/types.h"

int vk36n16i_read_key(struct vk36n16i_data *data, uint16_t *key_data) {
    struct i2c_client *client = data->client;

    int ret;
    uint8_t buf[2];
    struct i2c_msg msg;

    msg.addr = client->addr;
    msg.flags = I2C_M_RD;
    msg.buf = buf;
    msg.len = sizeof(buf);

    ret = i2c_transfer(client->adapter, &msg, 1);
    if (ret < 0) {
        return ret;
    }
    if (ret != 1) {
        return -EIO;
    }

    *key_data = (uint16_t)(buf[0]) + (uint16_t)(buf[1] << 8);
    return 0;
}
