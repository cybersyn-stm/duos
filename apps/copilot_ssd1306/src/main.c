#include "device.h"
#include "kmod_utils.h"
#include "menu.h"
#include "ssd1306.h"
#include "ssd1306_8x16.h"
#include "ui.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
uint8_t file_buffer[BUFFER_SIZE];

device_information vk36n16i_device;
device_operation vk36n16i_ops;
device_information ssd1306_device;
device_operation ssd1306_ops;

// 确保驱动已加载，若未加载则尝试插入内核模块并等待设备节点
static int ensure_driver(const char *mod, const char *ko, const char *devnode) {
    int loaded = kmod_is_loaded(mod);
    if (loaded == 1)
        return 0;
    if (loaded < 0) {
        fprintf(stderr, "check module %s failed\n", mod);
        return -1;
    }
    fprintf(stderr, "loading %s from %s...\n", mod, ko);
    if (kmod_insmod_file(ko, "") < 0) {
        perror("insmod_file");
        return -1;
    }
    if (wait_devnode(devnode, 1500) < 0) {
        fprintf(stderr, "device node %s not found\n", devnode);
        return -1;
    }
    return 0;
}

int vk36n16i_fd() {
    vk36n16i_ops.open_device = open_vk36n16i;
    vk36n16i_ops.close_device = close_vk36n16i;
    vk36n16i_ops.read_device = input_read_key;
    vk36n16i_device.device_name = "/dev/vk36n16i";
    vk36n16i_device.device_id =
        vk36n16i_ops.open_device(vk36n16i_device.device_name);
    return vk36n16i_device.device_id;
}

int ssd1306_fd() {
    ssd1306_ops.open_device = open_ssd1306;
    ssd1306_ops.close_device = close_ssd1306;
    ssd1306_device.device_name = "/dev/ssd1306";
    ssd1306_device.device_id =
        ssd1306_ops.open_device(ssd1306_device.device_name);
    return ssd1306_device.device_id;
}

// 主程序入口，负责驱动加载、设备初始化、UI与菜单系统启动
int main() {
    // 加载vk36n16i驱动
    if (ensure_driver("vk36n16i", "/root/vk36n16i.ko", "/dev/vk36n16i") < 0)
        return 1;
    // 加载ssd1306驱动
    if (ensure_driver("ssd1306", "/root/ssd1306.ko", "/dev/ssd1306") < 0)
        return 1;
    printf("module loaded successfully\n");
    int key_value;
    // 初始化输入设备
    vk36n16i_fd();
    // 初始化显示设备
    ssd1306_fd();
    // 初始化UI结构体和样式
    struct ui ui;
    ui.buf = file_buffer;
    ui_menu_style_init(&ui.style);
    // 初始化菜单系统
    struct menu_system sys;
    menu_system_start(&sys, menu_root_get(), MENU_VISIBLE_ROWS);
    // 主循环：处理按键、渲染UI、发送显示缓冲区
    while (1) {
        // 动画模式下特殊处理（非阻塞输入，去掉usleep）
        if (sys.mode == MENU_MODE_MARIO_ANIM || sys.mode == MENU_MODE_STAR_ANIM || sys.mode == MENU_MODE_WAVE_ANIM) {
            key_value = vk36n16i_ops.read_device(&vk36n16i_device);
            if (key_value == MENU_KEY_BACK) {
                sys.mode = MENU_MODE_NORMAL;
            }
            ui_system_render(&ui, &sys);
            send_buffer(ui.buf, &ssd1306_device);
            continue;
        }
        key_value = vk36n16i_ops.read_device(&vk36n16i_device);
        menu_system_on_key(&sys, key_value);
        uint64_t t_render_start = now_ms();
        ui_system_render(&ui, &sys);
        uint64_t t_render_end = now_ms();
        printf("[PROFILE] 渲染耗时: %llu ms\n", (unsigned long long)(t_render_end - t_render_start));
        uint64_t t_send_start = now_ms();
        send_buffer(ui.buf, &ssd1306_device);
        uint64_t t_send_end = now_ms();
        printf("[PROFILE] 发送耗时: %llu ms\n", (unsigned long long)(t_send_end - t_send_start));
        // FPS模式下统计帧率
        if (sys.mode == MENU_MODE_FPS)
            fps_on_frame(&sys.fps, now_ms());
    }
}

