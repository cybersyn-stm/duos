#include "menu.h"
#include "input.h"
#include "linklisted.h"
#include "output.h"
#include "ssd1306.h"
#include "ssd1306_8x16.h"
#include "ui.h"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

buff_type file_buffer[FB_SIZE];

MenuCtx_t *menu_ctx;
InputCtx_t *input_ctx;
UICtx_t *ui_ctx;
FileOperation_t *file_op;
void func_callback(MenuCtx_t *menu) {}
void func_init(MenuCtx_t *menu) {
    ui_ctx->show_arrow = 0;
    ui_ctx->enable_fuc = 1;
    menu->function = func_callback;
}
static void password_func(MenuCtx_t *menu) {
    int key = menu->last_key_value;

    // 简单映射：1/2/3 -> '1'/'2'/'3'
    char ch = 0;
    if (key == 1)
        ch = '1';
    else if (key == 2)
        ch = '2';
    else if (key == 3)
        ch = '3';
    else
        return;

    if (menu->password_len < (int)sizeof(menu->password) - 1) {
        menu->password[menu->password_len++] = ch;
        menu->password[menu->password_len] = '\0';
    }
}
linklisted_t *create_linklist() {
    linklisted_t *main_menu = linklisted_create("Menu", NULL, NULL);
    linklisted_t *setting_menu = linklisted_create("UserName", NULL, NULL);
    linklisted_t *PassWord_menu =
        linklisted_create("Enter_Password", password_func, NULL);

    linklisted_add(main_menu, linklisted_create("Setting", NULL, setting_menu));
    linklisted_add(main_menu, linklisted_create("Wifi", NULL, NULL));
    linklisted_add(main_menu, linklisted_create("SSH", NULL, NULL));
    linklisted_add(main_menu, linklisted_create("View", NULL, NULL));
    linklisted_add(main_menu, linklisted_create("PLC", NULL, NULL));
    linklisted_add(main_menu, linklisted_create("Close", NULL, NULL));
    linklisted_add(main_menu, linklisted_create("Reboot", NULL, NULL));

    // Setting sublist menu
    linklisted_add(setting_menu,
                   linklisted_create("PassWord", NULL, PassWord_menu));
    return main_menu;
}
void init_struct() {
    menu_ctx = (MenuCtx_t *)malloc(sizeof(MenuCtx_t));
    input_ctx = (InputCtx_t *)malloc(sizeof(InputCtx_t));
    ui_ctx = (UICtx_t *)malloc(sizeof(UICtx_t));
    file_op = (FileOperation_t *)malloc(sizeof(FileOperation_t));

    menu_ctx->menu = create_linklist();
    menu_ctx->arrow_x = 0;
    menu_ctx->arrow_page = 0;
    menu_ctx->arrow_y = 0;
    menu_ctx->menu_len = linklist_len(menu_ctx->menu);

    ui_ctx->fb = file_buffer;
    ui_ctx->show_arrow = 1;
}

int open_hardware() {
    int screen_fd = open_ssd1306();
    file_op->screen_fd = screen_fd;
    if (screen_fd < 0) {
        return -1;
    }

    int key_fd = open_vk36n16i();
    file_op->key_fd = key_fd;
    input_ctx->fd = key_fd;
    if (key_fd < 0)
        return -1;
    return 0;
}

int close_hardware() {
    close_ssd1306(file_op->screen_fd);
    close_vk36n16i(file_op->key_fd);
    return 0;
}
int main() {
    init_struct();
    open_hardware();
    while (1) {
        input_key_process(input_ctx, menu_ctx, ui_ctx);
        ui_render(ui_ctx, menu_ctx);
        menu_send_buffer(ui_ctx, file_op);
    }
    close_hardware();
    return 0;
}
