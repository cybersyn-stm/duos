#ifndef UI_H
#define UI_H

#include "menu.h"
#include <stdint.h>

#define UI_COLS_8x16 16
#define UI_ROWS_8x16 4

struct ui_menu_style {
    int cols;           // 一行字符串数量
    int title_row;      // 标题行
    int first_item_row; // 第一行菜单项行
    int arrow_col;      // 箭头列
    int label_col;      // 菜单项标签列
    char arrow_char;    //>
    char *sub_mark_ch;  //<<
    int sub_mark_col;
};

struct ui {
    uint8_t *buf;
    struct ui_menu_style style;
};

void ui_menu_style_init(struct ui_menu_style *style);
void ui_system_render(struct ui *ui, struct menu_system *system);
#endif
