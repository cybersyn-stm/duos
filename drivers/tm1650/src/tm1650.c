#include "tm1650.h"
#include "linux/device.h"
#include "linux/module.h"
#include "linux/types.h"
#include "tm1650_misc.h"

int tm1650_write_byte(struct tm1650_system *sys, uint8_t cmd, uint8_t data) {
    struct i2c_client *client = sys->client;
    struct i2c_msg msg;
    int ret;

    msg.addr = cmd;
    msg.buf = &data;
    msg.flags = 0;
    msg.len = 1;

    ret = i2c_transfer(client->adapter, &msg, 1);
    if (ret < 0) {
        dev_err(&client->dev, "I2C write failed: %d\n", ret);
    }
    return 0;
}

static int tm1650_probe(struct i2c_client *client) {
    struct tm1650_system *sys;
    dev_info(&client->dev, "Probing TM1650 display driver\n");
    sys = devm_kzalloc(&client->dev, sizeof(struct tm1650_system), GFP_KERNEL);
    sys->client = client;
    i2c_set_clientdata(client, sys);
    tm1650_misc_init(sys);
    return 0;
}

static const struct i2c_device_id tm1650_id[] = {{"tm1650", 0}, {}};
MODULE_DEVICE_TABLE(i2c, tm1650_id);
static const struct of_device_id tm1650_of_match[] = {
    {.compatible = "tm1650"},
    {},
};
MODULE_DEVICE_TABLE(of, tm1650_of_match);

static struct i2c_driver tm1650_driver = {
    .driver =
        {
            .name = "tm1650",
            .of_match_table = tm1650_of_match,
        },
    .probe_new = tm1650_probe,
    .id_table = tm1650_id,
};

module_i2c_driver(tm1650_driver);

MODULE_AUTHOR("cybersyn");
MODULE_DESCRIPTION("TM1650 7-segment display driver");
MODULE_LICENSE("GPL");
