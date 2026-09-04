#include "device.h"
#include "list.h"
#include "ssd1306.h"
#include "ssd1306_8x16.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* ---------- Key mapping (customize here) ---------- */
#define KEY_UP 10
#define KEY_DOWN 11
#define KEY_OK 12
#define KEY_BACK 13

/* OLED layout for 8x16 font */
#define OLED_COLS (SSD1306_W / 8)  /* 16 chars */
#define OLED_ROWS (SSD1306_H / 16) /* 4 rows */
#define ITEM_ROWS (OLED_ROWS - 1)  /* 3 item rows when title shown */

/* Framebuffer */
static uint8_t fb[SSD1306_file_buff_SIZE];

/* ---------- Devices (reuse your current ops style) ---------- */
static device_information vk36n16i_device;
static device_operation vk36n16i_ops;
static device_information ssd1306_device;
static device_operation ssd1306_ops;

static int vk36n16i_fd(void) {
    vk36n16i_ops.open_device = open_vk36n16i;
    vk36n16i_ops.close_device = close_vk36n16i;
    vk36n16i_ops.read_device = input_read_key;
    vk36n16i_device.device_name = "/dev/vk36n16i";
    vk36n16i_device.device_id =
        vk36n16i_ops.open_device(vk36n16i_device.device_name);
    return vk36n16i_device.device_id;
}

static int ssd1306_fd(void) {
    ssd1306_ops.open_device = open_ssd1306;
    ssd1306_ops.close_device = close_ssd1306;
    ssd1306_device.device_name = "/dev/ssd1306";
    ssd1306_device.device_id =
        ssd1306_ops.open_device(ssd1306_device.device_name);
    return ssd1306_device.device_id;
}

/* ---------- Menu model (intrusive list) ---------- */
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
    menu_item_t *selected;
    int scroll; /* first visible index */
} menu_state_t;

/* ---------- Menu helpers ---------- */
static void menu_init(menu_t *m, const char *title, menu_t *parent) {
    m->title = title;
    m->parent = parent;
    INIT_LIST_HEAD(&m->items);
}

static void menu_item_init_action(menu_item_t *it, const char *label,
                                  menu_action_cb cb) {
    it->label = label;
    it->type = MENU_ITEM_ACTION;
    it->action = cb;
    it->child = NULL;
    INIT_LIST_HEAD(&it->node);
}

static void menu_item_init_submenu(menu_item_t *it, const char *label,
                                   menu_t *child) {
    it->label = label;
    it->type = MENU_ITEM_SUBMENU;
    it->action = NULL;
    it->child = child;
    INIT_LIST_HEAD(&it->node);
}

static void menu_add_item(menu_t *m, menu_item_t *it) {
    list_add_tail(&it->node, &m->items);
}

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

static menu_item_t *menu_last_item(menu_t *m) {
    if (list_empty(&m->items))
        return NULL;
    return list_entry(m->items.prev, menu_item_t, node);
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

static menu_item_t *menu_item_at(menu_t *m, int index) {
    int i = 0;
    struct list_head *pos;
    list_for_each(pos, &m->items) {
        if (i == index)
            return list_entry(pos, menu_item_t, node);
        i++;
    }
    return NULL;
}

static void menu_state_enter(menu_state_t *st, menu_t *root) {
    st->current = root;
    st->selected = menu_first_item(root);
    st->scroll = 0;
}

/* ---------- Rendering ---------- */
static void make_line(char out[OLED_COLS + 1], const char *s) {
    int i = 0;
    for (; i < OLED_COLS; i++) {
        if (!s || s[i] == '\0')
            break;
        out[i] = s[i];
    }
    for (; i < OLED_COLS; i++)
        out[i] = ' ';
    out[OLED_COLS] = '\0';
}

static void render_item_line(char out[OLED_COLS + 1], const menu_item_t *it,
                             int is_selected) {
    char tmp[OLED_COLS + 1];
    make_line(tmp, it ? it->label : "");

    /* prefix */
    out[0] = is_selected ? '>' : ' ';
    for (int i = 1; i < OLED_COLS; i++)
        out[i] = tmp[i - 1];
    out[OLED_COLS] = '\0';

    if (it && it->type == MENU_ITEM_SUBMENU) {
        out[OLED_COLS - 1] = '>'; /* submenu hint on right edge */
    }
}

static void ui_oled_render_menu(uint8_t *buffer, const menu_state_t *st) {
    ssd1306_buffer_clear(buffer);

    if (!st || !st->current)
        return;

    const menu_t *m = st->current;
    const int total = menu_count_items(m);

    int selected_index = -1;
    if (st->selected)
        selected_index =
            menu_index_of((menu_t *)m, (menu_item_t *)st->selected);

    /* clamp scroll */
    int scroll = st->scroll;
    if (scroll < 0)
        scroll = 0;
    if (total > ITEM_ROWS && scroll > total - ITEM_ROWS)
        scroll = total - ITEM_ROWS;
    if (total <= ITEM_ROWS)
        scroll = 0;

    /* title row (row 0) */
    char title[OLED_COLS + 1];
    make_line(title, m->title ? m->title : "");
    ssd1306_draw_buffer_string8x16(buffer, 0, 0, title);

    /* item rows (row 1..3) */
    for (int vis = 0; vis < ITEM_ROWS; vis++) {
        int idx = scroll + vis;
        menu_item_t *it =
            (idx >= 0 && idx < total) ? menu_item_at((menu_t *)m, idx) : NULL;

        char line[OLED_COLS + 1];
        render_item_line(line, it, (idx == selected_index));
        ssd1306_draw_buffer_string8x16(buffer, 0, 1 + vis, line);
    }
}

/* ---------- Actions ---------- */
static void show_run_message(const char *name) {
    /* simple feedback on OLED title line */
    char t[OLED_COLS + 1];
    char msg[64];
    snprintf(msg, sizeof(msg), "RUN %s", name ? name : "");
    make_line(t, msg);
    ssd1306_buffer_clear(fb);
    ssd1306_draw_buffer_string8x16(fb, 0, 0, t);
    send_buffer(fb, ssd1306_device.device_id);
    usleep(250 * 1000);
}

static void action_hello(int key_value) {
    (void)key_value;
    printf("Action: HELLO\n");
    show_run_message("HELLO");
}

static void action_about(int key_value) {
    (void)key_value;
    printf("Action: ABOUT\n");
    show_run_message("ABOUT");
}

static void action_brightness(int key_value) {
    (void)key_value;
    printf("Action: BRIGHT\n");
    show_run_message("BRIGHT");
}

/* ---------- Key handling ---------- */
static void ensure_selected_visible(menu_state_t *st) {
    if (!st || !st->current || !st->selected)
        return;

    const int total = menu_count_items(st->current);
    if (total <= ITEM_ROWS) {
        st->scroll = 0;
        return;
    }

    int sel = menu_index_of(st->current, st->selected);
    if (sel < 0)
        sel = 0;

    if (sel < st->scroll)
        st->scroll = sel;
    if (sel >= st->scroll + ITEM_ROWS)
        st->scroll = sel - (ITEM_ROWS - 1);

    if (st->scroll < 0)
        st->scroll = 0;
    if (st->scroll > total - ITEM_ROWS)
        st->scroll = total - ITEM_ROWS;
}

static void menu_handle_key(menu_state_t *st, int key) {
    if (!st || !st->current)
        return;

    if (list_empty(&st->current->items))
        return;
    if (!st->selected)
        st->selected = menu_first_item(st->current);

    if (key == KEY_UP) {
        /* move to prev, wrap */
        if (&st->selected->node == st->current->items.next) {
            st->selected = menu_last_item(st->current);
        } else {
            st->selected =
                list_entry(st->selected->node.prev, menu_item_t, node);
        }
        ensure_selected_visible(st);
    } else if (key == KEY_DOWN) {
        /* move to next, wrap */
        if (&st->selected->node == st->current->items.prev) {
            st->selected = menu_first_item(st->current);
        } else {
            st->selected =
                list_entry(st->selected->node.next, menu_item_t, node);
        }
        ensure_selected_visible(st);
    } else if (key == KEY_OK) {
        if (st->selected->type == MENU_ITEM_SUBMENU && st->selected->child) {
            st->current = st->selected->child;
            st->selected = menu_first_item(st->current);
            st->scroll = 0;
        } else if (st->selected->type == MENU_ITEM_ACTION &&
                   st->selected->action) {
            st->selected->action(key);
        }
    } else if (key == KEY_BACK) {
        if (st->current->parent) {
            st->current = st->current->parent;
            /* keep selection simple: select first item when returning */
            st->selected = menu_first_item(st->current);
            st->scroll = 0;
        }
    }
}

/* ---------- Build menu tree ---------- */
/* ---------- Build menu tree (expanded) ---------- */
static menu_t menu_main;
static menu_t menu_settings;
static menu_t menu_tools;

/* third-level menus */
static menu_t menu_display;
static menu_t menu_system;

/* main items */
static menu_item_t main_hello;
static menu_item_t main_settings;
static menu_item_t main_tools;
static menu_item_t main_about;
static menu_item_t main_reboot;
static menu_item_t main_sleep;

/* settings items */
static menu_item_t settings_display;
static menu_item_t settings_system;
static menu_item_t settings_back_hint;

/* tools items */
static menu_item_t tools_test_keys;
static menu_item_t tools_draw_demo;
static menu_item_t tools_back_hint;

/* display items (3rd level) */
static menu_item_t display_brightness;
static menu_item_t display_contrast;
static menu_item_t display_invert;
static menu_item_t display_back_hint;

/* system items (3rd level) */
static menu_item_t system_info;
static menu_item_t system_factory_reset;
static menu_item_t system_back_hint;

/* --- actions --- */
static void action_back_hint(int key_value) {
    (void)key_value;
    printf("Hint: use BACK key\n");
    show_run_message("USE BACK");
}

static void action_test_keys(int key_value) {
    (void)key_value;
    printf("Tool: KEY TEST\n");
    show_run_message("KEY TEST");
}

static void action_draw_demo(int key_value) {
    (void)key_value;
    printf("Tool: DRAW DEMO\n");
    show_run_message("DRAW DEMO");
}

static void action_reboot(int key_value) {
    (void)key_value;
    printf("Action: REBOOT\n");
    show_run_message("REBOOT");
}

static void action_sleep(int key_value) {
    (void)key_value;
    printf("Action: SLEEP\n");
    show_run_message("SLEEP");
}

static void action_contrast(int key_value) {
    (void)key_value;
    printf("Action: CONTRAST\n");
    show_run_message("CONTRAST");
}

static void action_invert(int key_value) {
    (void)key_value;
    printf("Action: INVERT\n");
    show_run_message("INVERT");
}

static void action_sysinfo(int key_value) {
    (void)key_value;
    printf("Action: SYSINFO\n");
    show_run_message("SYSINFO");
}

static void action_factory_reset(int key_value) {
    (void)key_value;
    printf("Action: RESET\n");
    show_run_message("RESET");
}

/* build the whole tree */
static void build_menu(void) {
    /* menus */
    menu_init(&menu_main, "MAIN", NULL);

    menu_init(&menu_settings, "SETTINGS", &menu_main);
    menu_init(&menu_tools, "TOOLS", &menu_main);

    menu_init(&menu_display, "DISPLAY", &menu_settings);
    menu_init(&menu_system, "SYSTEM", &menu_settings);

    /* MAIN items (more than 3 to test scrolling) */
    menu_item_init_action(&main_hello, "Hello", action_hello);
    menu_item_init_submenu(&main_settings, "Settings", &menu_settings);
    menu_item_init_submenu(&main_tools, "Tools", &menu_tools);
    menu_item_init_action(&main_about, "About", action_about);
    menu_item_init_action(&main_reboot, "Reboot", action_reboot);
    menu_item_init_action(&main_sleep, "Sleep", action_sleep);

    menu_add_item(&menu_main, &main_hello);
    menu_add_item(&menu_main, &main_settings);
    menu_add_item(&menu_main, &main_tools);
    menu_add_item(&menu_main, &main_about);
    menu_add_item(&menu_main, &main_reboot);
    menu_add_item(&menu_main, &main_sleep);

    /* SETTINGS items (enter 3rd level) */
    menu_item_init_submenu(&settings_display, "Display", &menu_display);
    menu_item_init_submenu(&settings_system, "System", &menu_system);
    menu_item_init_action(&settings_back_hint, "Back (use key)",
                          action_back_hint);

    menu_add_item(&menu_settings, &settings_display);
    menu_add_item(&menu_settings, &settings_system);
    menu_add_item(&menu_settings, &settings_back_hint);

    /* TOOLS items */
    menu_item_init_action(&tools_test_keys, "Key Test", action_test_keys);
    menu_item_init_action(&tools_draw_demo, "Draw Demo", action_draw_demo);
    menu_item_init_action(&tools_back_hint, "Back (use key)", action_back_hint);

    menu_add_item(&menu_tools, &tools_test_keys);
    menu_add_item(&menu_tools, &tools_draw_demo);
    menu_add_item(&menu_tools, &tools_back_hint);

    /* DISPLAY (3rd level) items */
    menu_item_init_action(&display_brightness, "Brightness", action_brightness);
    menu_item_init_action(&display_contrast, "Contrast", action_contrast);
    menu_item_init_action(&display_invert, "Invert", action_invert);
    menu_item_init_action(&display_back_hint, "Back (use key)",
                          action_back_hint);

    menu_add_item(&menu_display, &display_brightness);
    menu_add_item(&menu_display, &display_contrast);
    menu_add_item(&menu_display, &display_invert);
    menu_add_item(&menu_display, &display_back_hint);

    /* SYSTEM (3rd level) items */
    menu_item_init_action(&system_info, "Info", action_sysinfo);
    menu_item_init_action(&system_factory_reset, "Factory Reset",
                          action_factory_reset);
    menu_item_init_action(&system_back_hint, "Back (use key)",
                          action_back_hint);

    menu_add_item(&menu_system, &system_info);
    menu_add_item(&menu_system, &system_factory_reset);
    menu_add_item(&menu_system, &system_back_hint);
}
/* ---------- Main loop ---------- */
int main(void) {
    int key_value;

    if (vk36n16i_fd() < 0) {
        perror("vk36n16i open failed");
        return 1;
    }

    if (ssd1306_fd() < 0) {
        perror("ssd1306 open failed");
        return 1;
    }

    build_menu();

    menu_state_t st;
    menu_state_enter(&st, &menu_main);

    /* first render */
    ui_oled_render_menu(fb, &st);
    send_buffer(fb, ssd1306_device.device_id);

    while (1) {
        key_value = vk36n16i_ops.read_device(&vk36n16i_device);

        if (key_value != -1) {
            /* only handle our control keys; numbers 1..9 are ignored for now */
            if (key_value == KEY_UP || key_value == KEY_DOWN ||
                key_value == KEY_OK || key_value == KEY_BACK) {

                menu_handle_key(&st, key_value);
                ui_oled_render_menu(fb, &st);
                send_buffer(fb, ssd1306_device.device_id);
            } else {
                /* debug: print numeric keys */
                if (key_value >= 1 && key_value <= 9) {
                    printf("number key: %d\n", key_value);
                } else {
                    printf("key value: %d\n", key_value);
                }
            }
        } else {
            /* avoid busy loop when device is non-blocking */
            usleep(10 * 1000);
        }
    }

    return 0;
}
