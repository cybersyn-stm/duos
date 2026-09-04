#include "input.h"
#include "menu.h"
#include "ui.h"
#include <stdio.h>
#include <unistd.h>

static MenuState_t menu_stack[MENU_STACK_DEPTH];
static MenuState_t menu_exit_stack[MENU_STACK_DEPTH];

static int menu_stack_top = 0;
static int menu_exit_stack_top = 0;

int open_vk36n16i() {
    int fd = open("/dev/vk36n16i", O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("open key device fail");
        return -1;
    }
    return fd;
}

int close_vk36n16i(int fd) {
    if (close(fd) < 0) {
        perror("close key device fail");
        return -1;
    }
    return 0;
}
int input_read_key(InputCtx_t *in) {
    int fd = in->fd;
    int key_value = 0;
    int rd;
    if (fd < 0) {
        perror("open key device fail");
        return -1;
    }
    rd = read(fd, &key_value, sizeof(int));
    if (rd == -1) {
        return -1;
    }
    return key_value;
}
static int push_menu_state(linklisted_t *menu, int arrow_y, int arrow_page,
                           int menu_len, int show_arrow) {
    if (menu_stack_top >= MENU_STACK_DEPTH)
        return -1;
    menu_stack[menu_stack_top].menu = menu;
    menu_stack[menu_stack_top].arrow_y = arrow_y;
    menu_stack[menu_stack_top].arrow_page = arrow_page;
    menu_stack[menu_stack_top].menu_len = menu_len;
    menu_stack[menu_stack_top].show_arrow = show_arrow;
    menu_stack_top++;
    return 0;
}

static int pop_menu_state(linklisted_t **menu, int *arrow_y, int *arrow_page,
                          int *menu_len, int *show_arrow) {
    if (menu_stack_top <= 0)
        return -1;
    menu_stack_top--;
    *menu = menu_stack[menu_stack_top].menu;
    *arrow_y = menu_stack[menu_stack_top].arrow_y;
    *arrow_page = menu_stack[menu_stack_top].arrow_page;
    *menu_len = menu_stack[menu_stack_top].menu_len;
    *show_arrow = menu_stack[menu_stack_top].show_arrow;

    return 0;
}

static int push_exit_menu_state(linklisted_t *menu, int arrow_y, int arrow_page,
                                int menu_len, int show_arrow) {
    if (menu_exit_stack_top >= MENU_STACK_DEPTH)
        return -1;
    menu_exit_stack[menu_exit_stack_top].menu = menu;
    menu_exit_stack[menu_exit_stack_top].arrow_y = arrow_y;
    menu_exit_stack[menu_exit_stack_top].arrow_page = arrow_page;
    menu_exit_stack[menu_exit_stack_top].menu_len = menu_len;
    menu_exit_stack[menu_exit_stack_top].show_arrow = show_arrow;
    menu_exit_stack_top++;
    return 0;
}
static int pop_exit_menu_state(linklisted_t **menu, int *arrow_y,
                               int *arrow_page, int *menu_len,
                               int *show_arrow) {
    if (menu_exit_stack_top <= 0)
        return -1;
    menu_exit_stack_top--;
    *menu = menu_exit_stack[menu_exit_stack_top].menu;
    *arrow_y = menu_exit_stack[menu_exit_stack_top].arrow_y;
    *arrow_page = menu_exit_stack[menu_exit_stack_top].arrow_page;
    *menu_len = menu_exit_stack[menu_exit_stack_top].menu_len;
    *show_arrow = menu_exit_stack[menu_exit_stack_top].show_arrow;

    return 0;
}
void input_key_process(InputCtx_t *in, MenuCtx_t *menu_ctx, UICtx_t *ui_ctx) {
    int key_value = input_read_key(in);
    if (key_value == 1 && menu_ctx->arrow_y < menu_ctx->menu_len - 1) {
        menu_ctx->arrow_y++;
        if (menu_ctx->arrow_page < 3) {
            menu_ctx->arrow_page++;
        } else if (menu_ctx->arrow_y < menu_ctx->menu_len) {
            menu_ctx->menu = linklisted_slide(menu_ctx->menu, 1);
        }
    } else if (key_value == 2 && menu_ctx->arrow_y > 0) {
        menu_ctx->arrow_y--;
        if (menu_ctx->arrow_page > 0) {
            menu_ctx->arrow_page--;
        } else if (menu_ctx->arrow_y >= 0) {
            menu_ctx->menu = linklisted_slide(menu_ctx->menu, -1);
        }
    } else if (key_value == 3 && menu_ctx->current_menu->sublist != NULL) {

        if (push_menu_state(menu_ctx->menu, menu_ctx->arrow_y,
                            menu_ctx->arrow_page, menu_ctx->menu_len,
                            ui_ctx->show_arrow) == 0) {

            menu_ctx->menu = menu_ctx->current_menu->sublist;
            menu_ctx->arrow_y = 0;
            menu_ctx->arrow_page = 0;
            pop_exit_menu_state(&menu_ctx->menu, &menu_ctx->arrow_y,
                                &menu_ctx->arrow_page, &menu_ctx->menu_len,
                                &ui_ctx->show_arrow);
            menu_ctx->menu_len = linklist_len(menu_ctx->menu);
        }
        if (menu_ctx->current_menu->sublist->function != NULL) {
            menu_ctx->current_menu->sublist->function(menu_ctx);
        }
    } else if (key_value == 4 && menu_ctx->menu->parentlist != NULL) {
        linklisted_t *prev_menu;
        int arrow_y, arrow_page, menu_len, show_arrow;
        if (pop_menu_state(&prev_menu, &arrow_y, &arrow_page, &menu_len,
                           &show_arrow) == 0) {
            push_exit_menu_state(menu_ctx->menu, menu_ctx->arrow_y,
                                 menu_ctx->arrow_page, menu_ctx->menu_len,
                                 ui_ctx->show_arrow);
            menu_ctx->menu = prev_menu;
            menu_ctx->arrow_y = arrow_y;
            menu_ctx->arrow_page = arrow_page;
            menu_ctx->menu_len = menu_len;
            ui_ctx->show_arrow = show_arrow;
        }
    }
}
// set str in display mind
