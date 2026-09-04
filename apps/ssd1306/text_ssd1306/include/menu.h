#ifndef __MENU_H__
#define __MENU_H__

#include "linklisted.h"
#include "ssd1306.h"

typedef enum {
    UI_MODE_MENU = 0, // 正常菜单模式：上下/进入/返回
    UI_MODE_FUNC = 1  // 功能模式：按键交给当前功能处理
} UiMode_t;

// ctx == context
typedef struct {
    int fd;
    int last_key;
} InputCtx_t;

typedef struct {
    buff_type *fb;
    int show_arrow;
    int enable_fuc;
} UICtx_t;

typedef struct MenuCtx_t {
    linklisted_t *menu;
    linklisted_t *current_menu;
    int arrow_y;
    int arrow_x;
    int arrow_page;
    int menu_len;

    // 当前选中项的功能（仅由 input 进入时设置，不由 ui_render 设置）
    void (*function)(struct MenuCtx_t *);

    // 新增：模式
    UiMode_t mode;

    // 新增：PassWord 功能用的输入状态（示例）
    char password[16];
    int password_len;

    // 新增：最近一次按键值，供 func_callback 读取
    int last_key_value;

} MenuCtx_t;

typedef struct {
    int key_fd;
    int screen_fd;
} FileOperation_t;

#endif
