#ifndef __INPUT_H__
#define __INPUT_H__

#include "menu.h"

#define MENU_STACK_DEPTH 8
typedef struct {
    linklisted_t *menu;
    int arrow_y;
    int arrow_page;
    int menu_len;
    int show_arrow;
} MenuState_t;

int input_read_key(InputCtx_t *in);

void input_key_process(InputCtx_t *in, MenuCtx_t *menu_ctx, UICtx_t *ui_ctx);

int open_vk36n16i(void);

int close_vk36n16i(int fd);
#endif
