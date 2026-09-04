#ifndef UI_MENU_H
#define UI_MENU_H

#include "list.h"
#include "ssd1306_8x16.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*menu_action_cb)(int key_value);

typedef enum {
    MENU_ITEM_ACTION = 0,
    MENU_ITEM_SUBMENU = 1,
} menu_item_type_t;

struct menu;

typedef struct menu_item {
    const char *label;
    menu_item_type_t type;

    /* if ACTION */
    menu_action_cb action;

    /* if SUBMENU */
    struct menu *child;

    /* intrusive link */
    struct list_head node;
} menu_item_t;

typedef struct menu {
    const char *title;
    struct menu *parent;
    struct list_head items; /* menu_item_t.node */
} menu_t;

typedef struct menu_state {
    menu_t *current;

    /* selection state in current menu */
    menu_item_t *selected;

    /* first visible index for scrolling */
    int scroll;

    /* layout */
    int show_title; /* 0/1 */
} menu_state_t;

/* init helpers */
void menu_init(menu_t *m, const char *title, menu_t *parent);
void menu_item_init_action(menu_item_t *it, const char *label,
                           menu_action_cb cb);
void menu_item_init_submenu(menu_item_t *it, const char *label, menu_t *child);
void menu_add_item(menu_t *m, menu_item_t *it);

/* state helpers */
void menu_state_enter(menu_state_t *st, menu_t *root);

/* UI rendering */
void ui_oled_render_menu(uint8_t *fb, const menu_state_t *st);

#ifdef __cplusplus
}
#endif

#endif /* UI_MENU_H */
