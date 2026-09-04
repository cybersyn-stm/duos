#include "ui.h"
#include "menu.h"
#include "password.h"
#include "ssd1306_8x16.h"
#include <stddef.h>
#include <stdio.h>

void ui_menu_style_init(struct ui_menu_style *ui) {
    if (ui == NULL)
        return;
    ui->cols = UI_COLS_8x16;
    ui->title_row = 0;
    ui->first_item_row = 1;
    ui->arrow_col = 0;
    ui->label_col = 2;
    ui->arrow_char = '>';
    ui->sub_mark_ch = ">";
    ui->sub_mark_col = ui->cols - 1;
}

static void ui_draw_str8x16(uint8_t *fb, int col, int row, const char *s) {
    int x;

    if (!fb || !s)
        return;

    /* col/row 是字符网格坐标 */
    x = col * 8;
    ssd1306_draw_buffer_string8x16(fb, x, row, s);
}

static void ui_draw_ch8x16(uint8_t *fb, int col, int row, char ch) {
    int x;

    if (!fb)
        return;

    x = col * 8;
    ssd1306_draw_buffer_char8x16(fb, x, row * 2, ch);
}

void ui_render_menu(struct ui *ui, const struct menu_view *v) {
    ssd1306_buffer_clear(ui->buf);

    // 标题渲染
    ui_draw_str8x16(ui->buf, 0, ui->style.title_row, v->title);

    // 菜单渲染
    int i;
    for (i = 0; i < MENU_VISIBLE_ROWS; i++) {
        // 获取当前行的菜单项
        const struct menu_item *it = v->items[i];
        int row = ui->style.first_item_row + i;
        // 如果菜单不存在，跳过这次渲染
        if (it == NULL)
            continue;
        // 文本渲染
        ui_draw_str8x16(ui->buf, ui->style.label_col, row, it->label);

        if (it->type == MENU_ITEM_SUB && ui->style.sub_mark_ch &&
            ui->style.sub_mark_ch) {
            // 子菜单标记渲染
            ui_draw_str8x16(ui->buf, ui->style.sub_mark_col, row,
                            ui->style.sub_mark_ch);
        }
        if (it->type == MENU_ITEM_ACT) {
            // 菜单项标记渲染
            if (it->action != NULL) {
                ui_draw_str8x16(ui->buf, ui->style.sub_mark_col, row,
                                ui->style.sub_mark_ch);
            }
            if (it->action == NULL) {
                // 菜单项不可用标记渲染
                ui_draw_str8x16(ui->buf, ui->style.sub_mark_col, row, "x");
            }
        }
    }
    if (v->sel_row >= 0 && v->sel_row < MENU_VISIBLE_ROWS) {
        int row = ui->style.first_item_row + v->sel_row;

        ui_draw_ch8x16(ui->buf, ui->style.arrow_col, row, ui->style.arrow_char);
    }
}

void ui_render_password(struct ui *ui, const struct password_view *view) {
    if (ui == NULL || view == NULL)
        return;

    ssd1306_buffer_clear(ui->buf);

    ui_draw_str8x16(ui->buf, 0, 0, view->title);
    ui_draw_str8x16(ui->buf, 0, 1, view->entered);
    if (view->len < MAX_ENTER_WORD) {
        ui_draw_str8x16(ui->buf, view->len, 1, "_");
    }
    ui_draw_str8x16(ui->buf, 0, 3, "OK=Save BK=Del");
}

static void ui_render_fps(struct ui *ui, const struct fps_system *fps) {
    char line[32];

    ssd1306_buffer_clear(ui->buf);

    ui_draw_str8x16(ui->buf, 0, 0, "FPS");
    snprintf(line, sizeof(line), "%lu", (unsigned long)fps->last_fps);
    ui_draw_str8x16(ui->buf, 0, 1, line);
    ui_draw_str8x16(ui->buf, 0, 3, "BACK=Exit");

    ui_draw_str8x16(ui->buf, fps->box_col, 2, "#");
}

// Mario像素动画帧（8x16）示例
static const uint8_t mario_sprite[16] = {
    0x18,0x18,0x3C,0x3C,0x7E,0x7E,0xDB,0xFF,
    0xFF,0x7E,0x7E,0x5A,0x5A,0x24,0x24,0x24
};

// 星星动画帧（8x8）
static const uint8_t star_sprite[8] = {
    0x18,0x3C,0x7E,0xDB,0xFF,0x7E,0x3C,0x18
};

// 绘制8x8星星
static void draw_star_to_ssd1306(uint8_t *buf, int x, int y) {
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (star_sprite[row] & (1 << (7 - col))) {
                int px = x + col;
                int py = y + row;
                if (px < SSD1306_W && py < SSD1306_H) {
                    int page = py / 8;
                    int bit = py % 8;
                    buf[page * SSD1306_W + px] |= (1 << bit);
                }
            }
        }
    }
}

// 星星动画：多颗星星左右漂浮
static void ui_render_star_anim(struct ui *ui, int frame) {
    ssd1306_buffer_clear(ui->buf);
    for (int i = 0; i < 5; i++) {
        int x = (frame * (i+1) * 3 + i*20) % (SSD1306_W - 8);
        int y = 8 + i * 10;
        draw_star_to_ssd1306(ui->buf, x, y);
    }
    ui_draw_str8x16(ui->buf, 0, 3, "BK=Exit");
}

// 波浪动画：正弦波
#include <math.h>
static void ui_render_wave_anim(struct ui *ui, int frame) {
    ssd1306_buffer_clear(ui->buf);
    for (int x = 0; x < SSD1306_W; x++) {
        int y = 32 + (int)(16 * sin((x + frame) * 0.1));
        if (y >= 0 && y < SSD1306_H) {
            int page = y / 8;
            int bit = y % 8;
            ui->buf[page * SSD1306_W + x] |= (1 << bit);
        }
    }
    ui_draw_str8x16(ui->buf, 0, 3, "BK=Exit");
}

// 将8x16点阵Mario像素正确写入SSD1306 buffer
static void draw_mario_to_ssd1306(uint8_t *buf, int x, int y) {
    // mario_sprite[row]每bit为一像素，row为Y方向
    for (int row = 0; row < 16; row++) {
        for (int col = 0; col < 8; col++) {
            if (mario_sprite[row] & (1 << (7 - col))) {
                int px = x + col;
                int py = y + row;
                if (px < SSD1306_W && py < SSD1306_H) {
                    int page = py / 8;
                    int bit = py % 8;
                    buf[page * SSD1306_W + px] |= (1 << bit);
                }
            }
        }
    }
}

static void ui_render_mario_anim(struct ui *ui, int frame) {
    ssd1306_buffer_clear(ui->buf);
    int x = (frame % (SSD1306_W - 8)); // Mario左右移动，防止越界
    int y = 16; // 居中显示
    draw_mario_to_ssd1306(ui->buf, x, y);
    ui_draw_str8x16(ui->buf, 0, 3, "BK=Exit");
}

void ui_system_render(struct ui *ui, struct menu_system *system) {
    if (ui == NULL || system == NULL)
        return;
    if (system->mode == MENU_MODE_NORMAL) {
        ui_render_menu(ui, &system->view);
    }
    if (system->mode == MENU_MODE_EDIT) {
        ui_render_password(ui, &system->pv);
    }
    if (system->mode == MENU_MODE_FPS) {
        ui_render_fps(ui, &system->fps);
    }
    if (system->mode == MENU_MODE_MARIO_ANIM) {
        static int mario_frame = 0;
        ui_render_mario_anim(ui, mario_frame++);
        if (mario_frame > 16) mario_frame = 0;
    }
    if (system->mode == MENU_MODE_STAR_ANIM) {
        static int star_frame = 0;
        ui_render_star_anim(ui, star_frame++);
        if (star_frame > 32) star_frame = 0;
    }
    if (system->mode == MENU_MODE_WAVE_ANIM) {
        static int wave_frame = 0;
        ui_render_wave_anim(ui, wave_frame++);
        if (wave_frame > 64) wave_frame = 0;
    }
}

