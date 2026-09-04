#include "vk36n16i.h"
#include "bash_exe.h"
#include "linux/module.h"
#include "vk36n16i_misc.h"
unsigned int poll_ms = 20;
module_param(poll_ms, uint, 0644); // 导出为模块参数，权限0644（rw-r--r--）
//=======================================================
//===================probe/remove函数====================
//=======================================================
static int vk36n16i_probe(struct i2c_client *client) {
    struct vk36n16i_data *data;
    int ret;
    dev_info(&client->dev, "Probing VK36N16I device\n");
    data = devm_kzalloc(&client->dev, sizeof(struct vk36n16i_data), GFP_KERNEL);
    if (data == NULL) {
        return -ENOMEM;
    }
    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->lock); // 必须添加这一行！
    duo_pinmux_init();

    ret = vk36n16i_misc_init(client);
    if (ret) {
        dev_err(&client->dev, "Failed to initialize misc device\n");
        return ret;
    }

    return 0;
}

static int vk36n16i_remove(struct i2c_client *client) {
    vk36n16i_misc_exit(client);
    dev_info(&client->dev, "Removed VK36N16I device\n");
    return 0;
}

// 设备树匹配表
static const struct of_device_id vk36n16i_of_match[] = {
    {.compatible = "vk36n16i"}, {}};
MODULE_DEVICE_TABLE(of, vk36n16i_of_match);
static const struct i2c_device_id vk36n16i_id[] = {{VK36N16I_NAME, 0}, {}};
MODULE_DEVICE_TABLE(i2c, vk36n16i_id);

static struct i2c_driver vk36n16i_driver = {
    .driver =
        {
            .name = VK36N16I_NAME,
            .of_match_table = vk36n16i_of_match,
        },
    .id_table = vk36n16i_id,
    .probe_new = vk36n16i_probe,
    .remove = vk36n16i_remove,
};

module_i2c_driver(vk36n16i_driver);

MODULE_AUTHOR("Cybersyn");
MODULE_DESCRIPTION("VK36N16I I2C Driver");
MODULE_LICENSE("GPL");
