#include "ui.h"
#include "ssd1306.h"
#include <stdio.h>
// render the menu to the screen
static linklisted_t *get_nth(linklisted_t *head, int n) {
    int i = 0;
    while (head && i < n) {
        head = head->next;
        i++;
    }
    return head;
}

void ui_render(UICtx_t *ui, MenuCtx_t *menu_ctx) {
    if (!ui || !menu_ctx || !ui->fb)
        return;

    if (menu_ctx->mode == UI_MODE_FUNC) {
        // 功能模式：示例渲染 PassWord 输入
        ssd1306_draw_buffer_string8x16(ui->fb, 0, 0, "Enter Password:");

        // 这里你可以显示 '*' 或明文，按需求
        char masked[sizeof(menu_ctx->password)];
        int n = menu_ctx->password_len;
        if (n < 0)
            n = 0;
        if (n > (int)sizeof(masked) - 1)
            n = (int)sizeof(masked) - 1;
        memset(masked, '*', (size_t)n);
        masked[n] = '\0';

        ssd1306_draw_buffer_string8x16(ui->fb, 0, 1, masked);
        return;
    }

    // 菜单模式：渲染菜单
    linklisted_t *current = menu_ctx->menu;
    int page = 0;
    int arrow_x_local = 0;

    // 根据 arrow_y 找选中项（假设 arrow_y 是当前menu上的index）
    linklisted_t *selected = get_nth(menu_ctx->menu, menu_ctx->arrow_y);

    while (current != NULL && page < (SSD1306_H / 16)) {
        int x = str_mind(current->name);

        if (current == selected) {
            arrow_x_local = x - 16;
        }

        ssd1306_draw_buffer_string8x16(ui->fb, x, page, current->name);
        current = current->next;
        page++;
    }

    if (arrow_x_local < 0)
        arrow_x_local = 0;
    if (arrow_x_local > (SSD1306_W - 8))
        arrow_x_local = SSD1306_W - 8;
    menu_ctx->arrow_x = arrow_x_local;

    if (ui->show_arrow) {
        ssd1306_draw_buffer_string8x16(ui->fb, menu_ctx->arrow_x,
                                       menu_ctx->arrow_page, "->");
    }
}
int str_mind(char *str) {
    int mind_retrun = 0;
    int str_long_size = strlen(str);

    mind_retrun = SSD1306_W - (str_long_size * 8);
    mind_retrun = mind_retrun / 2;
    return mind_retrun;
}

int linklist_len(linklisted_t *list) {
    int len = 0;
    linklisted_t *current = list;
    while (current != NULL) {
        len++;
        current = current->next;
    }
    return len;
}
