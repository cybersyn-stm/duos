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
static linklisted_t *get_nth(linklisted_t *head, int n) {
    int i = 0;
    while (head && i < n) {
        head = head->next;
        i++;
    }
    return head;
}

void input_key_process(InputCtx_t *in, MenuCtx_t *menu_ctx, UICtx_t *ui_ctx) {
    int key_value = input_read_key(in);
    if (key_value < 0)
        return;

    menu_ctx->last_key_value = key_value;

    // 功能模式：按键分发给功能
    if (menu_ctx->mode == UI_MODE_FUNC) {
        if (key_value == 4) {
            // Back：退出功能模式，回到菜单
            menu_ctx->mode = UI_MODE_MENU;
            ui_ctx->show_arrow = 1;
            ui_ctx->enable_fuc = 0; // 是否启用由你定义，这里仅示例
            return;
        }

        if (menu_ctx->function) {
            menu_ctx->function(menu_ctx);
        }
        return;
    }

    // 菜单模式：原有上下/进入/返回逻辑（略做整理）
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
    }

    // 更新 current_menu（由 input 负责，而不是 ui_render）
    menu_ctx->current_menu = get_nth(menu_ctx->menu, menu_ctx->arrow_y);

    if (key_value == 3 && menu_ctx->current_menu) {
        // Enter：优先进入 sublist
        if (menu_ctx->current_menu->sublist != NULL) {
            // 进入子菜单（沿用你现有 push/pop 逻辑也可以）
            menu_ctx->menu = menu_ctx->current_menu->sublist;
            menu_ctx->arrow_y = 0;
            menu_ctx->arrow_page = 0;
            menu_ctx->menu_len = linklist_len(menu_ctx->menu);
            menu_ctx->current_menu = get_nth(menu_ctx->menu, 0);
        } else if (menu_ctx->current_menu->function != NULL) {
            // 没有子菜单但有 function：进入功能模式
            menu_ctx->function = menu_ctx->current_menu->function;
            menu_ctx->mode = UI_MODE_FUNC;

            // 初始化密码输入状态（若是 PassWord 功能）
            menu_ctx->password_len = 0;
            menu_ctx->password[0] = '\0';

            ui_ctx->show_arrow = 0;
            ui_ctx->enable_fuc = 1;
        }
    } else if (key_value == 4 && menu_ctx->menu->parentlist != NULL) {
        // Back：返回上级（你原来是 pop_menu_state，这里略）
        // 这里建议继续用你原先的栈逻辑以恢复滚动位置
    }
}
// set str in display mind
