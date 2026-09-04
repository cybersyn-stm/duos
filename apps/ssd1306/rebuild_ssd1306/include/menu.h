#ifndef __MENU_H__
#define __MENU_H__

#include "linklisted.h"
#include "ssd1306.h"
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
    void (*function)(struct MenuCtx_t *);
} MenuCtx_t;

typedef struct {
    int key_fd;
    int screen_fd;
} FileOperation_t;

#endif // __MENU_H__
