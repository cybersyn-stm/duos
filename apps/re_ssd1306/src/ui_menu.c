#include "ui_menu.h"

#include <stdio.h>
#include <string.h>

#define OLED_COLS (SSD1306_W / 8)  /* 16 chars per row */
#define OLED_ROWS (SSD1306_H / 16) /* 4 rows for 8x16 font */
#define ITEMS_ROWS_WITHOUT_TITLE (OLED_ROWS)
#define ITEMS_ROWS_WITH_TITLE (OLED_ROWS - 1)

static int menu_count_items(const menu_t *m) {
    int n = 0;
    struct list_head *pos;
    list_for_each(pos, &m->items) n++;
    return n;
}

static menu_item_t *menu_first_item(menu_t *m) {
    if (list_empty(&m->items))
        return NULL;
    return list_entry(m->items.next, menu_item_t, node);
}

void menu_init(menu_t *m, const char *title, menu_t *parent) {
    m->title = title;
    m->parent = parent;
    INIT_LIST_HEAD(&m->items);
}

void menu_item_init_action(menu_item_t *it, const char *label,
                           menu_action_cb cb) {
    it->label = label;
    it->type = MENU_ITEM_ACTION;
    it->action = cb;
    it->child = NULL;
    INIT_LIST_HEAD(&it->node);
}

void menu_item_init_submenu(menu_item_t *it, const char *label, menu_t *child) {
    it->label = label;
    it->type = MENU_ITEM_SUBMENU;
    it->action = NULL;
    it->child = child;
    INIT_LIST_HEAD(&it->node);
}

void menu_add_item(menu_t *m, menu_item_t *it) {
    list_add_tail(&it->node, &m->items);
}

void menu_state_enter(menu_state_t *st, menu_t *root) {
    st->current = root;
    st->selected = menu_first_item(root);
    st->scroll = 0;
    st->show_title = 1;
}

static menu_item_t *menu_item_at(menu_t *m, int index) {
    int i = 0;
    struct list_head *pos;
    list_for_each(pos, &m->items) {
        if (i == index) {
            return list_entry(pos, menu_item_t, node);
        }
        i++;
    }
    return NULL;
}

static int menu_index_of(menu_t *m, menu_item_t *it) {
    int i = 0;
    struct list_head *pos;
    list_for_each(pos, &m->items) {
        if (pos == &it->node)
            return i;
        i++;
    }
    return -1;
}

/* truncate/copy to fixed width (OLED_COLS) */
static void make_line(char out[OLED_COLS + 1], const char *s) {
    int i;
    for (i = 0; i < OLED_COLS; i++) {
        if (!s || s[i] == '\0')
            break;
        out[i] = s[i];
    }
    for (; i < OLED_COLS; i++)
        out[i] = ' ';
    out[OLED_COLS] = '\0';
}

/* render one visible row of a menu item */
static void render_item_line(char out[OLED_COLS + 1], const menu_item_t *it,
                             int selected) {
    char tmp[OLED_COLS + 1];
    make_line(tmp, it ? it->label : "");

    /* Prefix: '>' selected, ' ' otherwise */
    out[0] = selected ? '>' : ' ';
    /* Copy rest (leave 1 column for prefix) */
    for (int i = 1; i < OLED_COLS; i++) {
        out[i] = tmp[i - 1];
    }
    out[OLED_COLS] = '\0';

    /* Optional hint for submenu */
    if (it && it->type == MENU_ITEM_SUBMENU) {
        out[OLED_COLS - 1] = '>'; /* right arrow hint */
    }
}

void ui_oled_render_menu(uint8_t *fb, const menu_state_t *st) {
    if (!fb || !st || !st->current)
        return;

    ssd1306_buffer_clear(fb);

    const menu_t *m = st->current;
    const int total = menu_count_items(m);
    const int selected_index =
        (st->selected ? menu_index_of((menu_t *)m, (menu_item_t *)st->selected)
                      : -1);

    const int item_rows =
        st->show_title ? ITEMS_ROWS_WITH_TITLE : ITEMS_ROWS_WITHOUT_TITLE;
    int scroll = st->scroll;

    /* clamp scroll */
    if (scroll < 0)
        scroll = 0;
    if (total > item_rows && scroll > total - item_rows)
        scroll = total - item_rows;
    if (total <= item_rows)
        scroll = 0;

    int row = 0;

    /* Title line */
    if (st->show_title) {
        char title[OLED_COLS + 1];
        make_line(title, m->title ? m->title : "");
        ssd1306_draw_buffer_string8x16(fb, 0, row, title);
        row++;
    }

    /* Items lines */
    for (int vis = 0; vis < item_rows; vis++) {
        const int idx = scroll + vis;

        menu_item_t *it =
            (idx >= 0 && idx < total) ? menu_item_at((menu_t *)m, idx) : NULL;

        char line[OLED_COLS + 1];
        render_item_line(line, it, (idx == selected_index));
        ssd1306_draw_buffer_string8x16(fb, 0, row, line);
        row++;
    }

    /* Optional scrollbar / page indicator could be drawn later */
}
