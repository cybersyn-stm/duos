#ifndef MENU_H
#define MENU_H

#include "fps.h"
#include "list.h"
#include "password.h"

#define MENU_VISIBLE_ROWS 3

//===================struct===================
enum menu_key {
    MENU_KEY_UP = 10,
    MENU_KEY_DOWN = 13,
    MENU_KEY_OK = 11,
    MENU_KEY_BACK = 14,
};


enum menu_item_type {
    MENU_ITEM_ACT = 0,
    MENU_ITEM_SUB,
};

typedef void (*menu_action_t)(void *ctx);

struct menu;

struct menu_item {
    const char *label;        // 菜单项文本
    enum menu_item_type type; // 菜单项类型：操作或子菜单

    menu_action_t action; // 操作类型的回调函数
    void *ctx;            // 操作类型的上下文数据

    struct menu *child; // 子菜单指针，仅当 type == MENU_ITEM_SUB 时有效

    struct list_head node; // 链表节点，用于将菜单项链接到菜单的 items 列表中
};

struct menu {
    const char *title;   // 菜单标题
    struct menu *parent; // 父菜单指针，根菜单的 parent 为 NULL

    /* 页面记忆：返回父菜单时恢复 */
    struct menu_item *last_sel; // 上次选中的菜单项指针
    int last_scroll;            // 上次滚动位置，用于返回父菜单时恢复滚动状态

    struct list_head items; // 菜单项链表头，包含该菜单的所有菜单项
};

struct menu_state {
    struct menu *current;       // 当前菜单指针
    struct menu_item *selected; // 当前选中的菜单项指针
    int scroll;                 // 菜单显示滚动起点
};

struct menu_view {
    /*菜单标题*/
    const char *title;

    /* 当前窗口（最多 3 条） */
    // menu_item 指针数组，指向当前窗口显示的菜单项，最多显示 MENU_VISIBLE_ROWS
    const struct menu_item *items[MENU_VISIBLE_ROWS];
    int sel_row; /* 0..2, 没有选中则 -1 */

    /* 可选：用于滚动条/页码等 */
    int total;
    int scroll;
};
// nav = navigation导航
struct menu_nav {
    struct menu_state st;
    int visible_rows;
};

enum menu_system_mode {
    MENU_MODE_NORMAL = 0,
    MENU_MODE_EDIT,
    MENU_MODE_FPS,
    MENU_MODE_MARIO_ANIM, // Mario动画模式
    MENU_MODE_STAR_ANIM,  // 星星动画模式
    MENU_MODE_WAVE_ANIM,  // 波浪动画模式
};

struct menu_system {
    struct menu_nav navigation;
    struct menu_view view;
    // password_system
    struct password_system pw;
    struct password_view pv;
    // fps_system
    struct fps_system fps;
    enum menu_system_mode mode;
};
// 菜单根节点，所有菜单的入口
struct menu *menu_root_get(void);

//===================API===================
void menu_init(struct menu *m, const char *title, struct menu *parent);
void menu_item_init_sub(struct menu_item *it, const char *label,
                        struct menu *child);
void menu_item_init_act(struct menu_item *it, const char *label,
                        menu_action_t action, void *ctx);
void menu_item_add(struct menu *m, struct menu_item *new_it);
void menu_build_view(struct menu_view *out, const struct menu_state *st,
                     int visible_rows);
void menu_system_start(struct menu_system *sys, struct menu *root,
                       int visile_rows);

void menu_on_key(struct menu_system *sys, enum menu_key key);

void menu_system_build_view(struct menu_view *out,
                            const struct menu_system *sys);
const struct menu_view *menu_system_view(const struct menu_system *sys);

void menu_system_on_key(struct menu_system *sys, int key_value);

// password API
#endif
