#include "ui.h"
#include "ssd1306.h"
#include <stdio.h>
// render the menu to the screen
void ui_render(UICtx_t *ui, MenuCtx_t *menu_ctx) {
    if (!ui || !menu_ctx || !ui->fb)
        return;

    linklisted_t *current = menu_ctx->menu;
    int page = 0;
    int arrow_x_local = 0;

    /* 渲染最多 SSD1306_H/16 行（每行 8x16 字体高度占两个 page） */
    while (current != NULL && page < (SSD1306_H / 16)) {
        int x = str_mind(current->name);
        if (menu_ctx->arrow_page == page) {
            /* 计算 arrow_x，但不要直接写回 menu_ctx->arrow_x（等待 clamp
             * 后再写） */
            arrow_x_local = x - 16;
            if (current->function != NULL) {
                menu_ctx->function = current->function;
            }
            menu_ctx->current_menu = current; // 保存当前菜单项指针
        }
        ssd1306_draw_buffer_string8x16(ui->fb, x, page, current->name);
        current = current->next;
        page++;
    }

    /* clamp arrow_x，使其在合法范围内，避免负值导致越界写 */
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
