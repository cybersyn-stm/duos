#include "menu.h"
#include "fps.h"
#include "password.h"
#include <stdio.h>
#include <unistd.h>
//========================API========================
// Mario动画菜单项回调
static void menu_action_mario_anim(void *ctx) {
    struct menu_system *sys = (struct menu_system *)ctx;
    sys->mode = MENU_MODE_MARIO_ANIM;
}

// 初始化菜单结构体
void menu_init(struct menu *m, const char *title, struct menu *parent) {
    // 写入保护
    if (m == NULL)
        return;
    // 写入标题和父菜单指针
    m->title = title;
    m->parent = parent;
    m->last_sel = NULL;
    m->last_scroll = 0;

    // 初始化菜单项链表
    INIT_LIST_HEAD(&m->items);
}

// 初始化子菜单类型的菜单项结构体
void menu_item_init_sub(struct menu_item *it, const char *label,
                        struct menu *child) {
    // 写入保护
    if (it == NULL)
        return;
    // 菜单项类型为子菜单
    it->type = MENU_ITEM_SUB;
    // 写入标题和子菜单指针
    it->label = label;
    it->child = child;
    // 非操作类型的菜单项，action 和 ctx 置空
    it->action = NULL;
    it->ctx = NULL;
    // 初始化链表节点
    INIT_LIST_HEAD(&it->node);
}

// 初始化操作类型的菜单项结构体
void menu_item_init_act(struct menu_item *it, const char *label,
                        menu_action_t action, void *ctx) {
    // 写入保护
    if (it == NULL)
        return;
    // 写入菜单项类型为操作
    it->type = MENU_ITEM_ACT;
    // 写入标题清空子菜单指针
    it->label = label;
    it->child = NULL;
    // 写入操作类型的回调函数和上下文数据
    it->action = action;
    it->ctx = ctx;
    // 初始化链表节点
    INIT_LIST_HEAD(&it->node);
}

void menu_item_add(struct menu *m, struct menu_item *new_it) {
    // 写入保护
    if (m == NULL || new_it == NULL)
        return;
    // 添加菜单项到菜单的链表中
    list_add_tail(&new_it->node, &m->items);
    if (new_it->type == MENU_ITEM_SUB && new_it->child != NULL)
        new_it->child->parent = m;
}

//=====================built-in menu=====================

// 新建一个菜单根节点，所有菜单的入口
static struct menu menu_main;
static struct menu menu_child;
static struct menu menu_movie; // 新增movie菜单
// 新建一个菜单项，关于菜单项
static struct menu_item it_about;
static struct menu_item it_about_1;
static struct menu_item it_about_2;
static struct menu_item child_menu;
static struct menu_item movie_menu; // movie菜单入口
static struct menu_item movie_info_1;
static struct menu_item movie_info_2;
static struct menu_item movie_info_3;
static struct menu_item mario_anim_menu; // Mario动画菜单项

static struct menu_item child_about;
static struct menu_item child_about_1;
static struct menu_item child_about_2;

// password menu
static struct menu_item it_password;
static struct menu_system *g_sys;

// fps menu
static struct menu_item it_fps;
// 菜单构建标志，防止重复构建菜单
static int menu_built;
// 测试用的关于菜单项回调函数
static void act_about(void *ctx) {
    (void)ctx;
    // 这里可以实现显示关于信息的功能
    // 例如：显示设备信息、版本号等
}

static void act_enter_password(void *ctx) {
    (void)ctx;

    if (g_sys == NULL)
        return;

    g_sys->mode = MENU_MODE_EDIT;
    password_enter(&g_sys->pw);
    password_system_build_view(&g_sys->pv, &g_sys->pw);
}

static void act_enter_fps(void *ctx) {
    (void)ctx;

    if (g_sys == NULL)
        return;

    g_sys->mode = MENU_MODE_FPS;
    fps_init(&g_sys->fps, now_ms());
}

// Mario动画菜单项回调，切换到动画模式
static void act_mario_anim(void *ctx) {
    if (g_sys == NULL)
        return;
    g_sys->mode = MENU_MODE_MARIO_ANIM;
}

// 星星动画菜单项回调
static void act_star_anim(void *ctx) {
    if (g_sys == NULL) return;
    g_sys->mode = MENU_MODE_STAR_ANIM;
}
// 波浪动画菜单项回调
static void act_wave_anim(void *ctx) {
    if (g_sys == NULL) return;
    g_sys->mode = MENU_MODE_WAVE_ANIM;
}

static void menu_build(void) {
    // 防止重复构建菜单
    if (menu_built)
        return;
    // 构建菜单树标志位
    menu_built = 1;

    // 一级菜单
    menu_init(&menu_main, "Main Menu", NULL);
    menu_item_init_act(&it_about, "act_about", NULL, NULL);
    menu_item_init_act(&it_about_1, "act_about_1", NULL, NULL);
    menu_item_init_act(&child_about, "child_about", NULL, NULL);
    menu_item_init_act(&it_about_2, "act_about_2", NULL, NULL);
    // password菜单
    menu_item_init_act(&it_password, "Password", act_enter_password, NULL);
    // fps菜单
    menu_item_init_act(&it_fps, "FPS", act_enter_fps, NULL);
    // 新增movie菜单
    menu_init(&menu_movie, "Movie Menu", NULL);
    menu_item_init_sub(&movie_menu, "Movie", &menu_movie);
    menu_item_init_act(&movie_info_1, "Inception", NULL, NULL);
    menu_item_init_act(&movie_info_2, "Interstellar", NULL, NULL);
    menu_item_init_act(&movie_info_3, "The Matrix", NULL, NULL);
    // 新增动画菜单项
    static struct menu_item star_anim_menu;
    static struct menu_item wave_anim_menu;
    menu_item_init_act(&mario_anim_menu, "Mario", act_mario_anim, NULL); // Mario动画菜单项
    menu_item_init_act(&star_anim_menu, "Star", act_star_anim, NULL);    // 星星动画
    menu_item_init_act(&wave_anim_menu, "Wave", act_wave_anim, NULL);    // 波浪动画
    // child_aboudc菜单
    menu_init(&menu_child, "Child menu", NULL);
    menu_item_init_sub(&child_menu, "child menu", &menu_child);
    menu_item_init_act(&child_about_1, "child_about_1", act_about, NULL);
    menu_item_init_act(&child_about_2, "child_about_2", act_about, NULL);

    menu_item_add(&menu_main, &it_about);
    menu_item_add(&menu_main, &it_about_1);
    menu_item_add(&menu_main, &it_about_2);
    menu_item_add(&menu_main, &child_menu);
    menu_item_add(&menu_main, &movie_menu); // 添加movie菜单入口

    menu_item_add(&menu_child, &child_about);
    menu_item_add(&menu_child, &child_about_1);
    menu_item_add(&menu_child, &child_about_2);

    menu_item_add(&menu_movie, &movie_info_1);
    menu_item_add(&menu_movie, &movie_info_2);
    menu_item_add(&menu_movie, &movie_info_3);
    menu_item_add(&menu_movie, &mario_anim_menu); // 添加Mario动画菜单项
    menu_item_add(&menu_movie, &star_anim_menu);  // 添加星星动画菜单项
    menu_item_add(&menu_movie, &wave_anim_menu);  // 添加波浪动画菜单项

    menu_item_add(&menu_main, &it_password);
    menu_item_add(&menu_main, &it_fps);
}

struct menu *menu_root_get(void) {
    // 确保菜单树已经构建
    menu_build();
    // 返回菜单根节点指针
    return &menu_main;
}

// 获取菜单的第一个菜单项指针，如果菜单为空或菜单指针为 NULL，则返回 NULL
static struct menu_item *menu_first_get(struct menu *m) {
    // 如果菜单指针为 NULL 或菜单项链表为空，则返回 NULL
    if (m == NULL || list_empty(&m->items))
        return NULL;

    // ptr是list_head指针，type是包含list_head的结构体类型，member是list_head在结构体中的成员名
    return container_of(m->items.next, struct menu_item, node);
}
// 获取菜单最后一个菜单指针
static struct menu_item *menu_last_get(struct menu *m) {
    // 写入保护
    if (m == NULL || list_empty(&m->items))
        return NULL;

    return container_of(m->items.prev, struct menu_item, node);
}

//==================menu operation==================

// 菜单项计数器
static int menu_count(struct menu *m) {
    // 写入保护
    if (m == NULL)
        return 0;
    // 统计菜单项数量
    int n = 0;
    struct list_head *current; // 指向当前菜单项的指针，循环用

    list_for_each(current, &m->items) { n++; }
    return n;
}

// 返回指定位置菜单项指针
static struct menu_item *menu_at(struct menu *m, int index) {
    // 写入保护
    if (m == NULL || index < 0)
        return NULL;

    int i = 0;
    struct list_head *current;

    list_for_each(current, &m->items) {
        if (i == index) {
            return container_of(current, struct menu_item, node);
        }
        i++;
    }

    return NULL;
}

// 返回指定菜单的位置
static int menu_index_of(struct menu *m, struct menu_item *it) {
    // 写入保护
    if (m == NULL || it == NULL)
        return -1;

    int i = 0;
    struct list_head *current;

    list_for_each(current, &m->items) {
        if (current == &it->node) {
            return i;
        }
        i++;
    }

    return -1;
}

//===============menu view================
// 渲染菜单视图核心函数
void menu_build_view(struct menu_view *out, const struct menu_state *st,
                     int visible_rows) {
    // 如果菜单视图指针为 NULL，则返回
    if (out == NULL)
        return;
    // 如果菜单状态指针为 NULL 或当前菜单指针为 NULL，则返回
    if (st == NULL || st->current == NULL)
        return;

    int i, total, sel_index, scroll;

    // 清除菜单视图内容
    out->title = NULL;
    out->sel_row = -1;
    out->total = 0;
    out->scroll = 0;
    // 清除菜单指针数组
    for (i = 0; i < MENU_VISIBLE_ROWS; i++) {
        out->items[i] = NULL;
    }
    // 确保可见行数不小于1，否则默认为 MENU_VISIBLE_ROWS
    if (visible_rows <= 0)
        visible_rows = MENU_VISIBLE_ROWS;

    // 判断当前菜单标题是否为空，如果为空则设置为默认标题
    out->title = st->current->title ? st->current->title : "";

    total = menu_count(st->current);
    out->total = total;
    // 记录scroll
    scroll = st->scroll;
    if (scroll < 0)
        scroll = 0;
    if (total <= visible_rows) {
        scroll = 0;
    } else if (scroll > total - visible_rows) {
        scroll = total - visible_rows;
    }

    out->scroll = scroll;
    // 构建菜单项指针数组，并记录选中行位置
    sel_index = menu_index_of(st->current, st->selected);

    for (i = 0; i < visible_rows; i++) {
        int idx = scroll + i;
        // 判断索引菜单是否存在，不存在则写入NULL
        out->items[i] = idx < total ? menu_at(st->current, idx) : NULL;
        // 如果当前索引等于选中菜单项索引，则记录选中行位置
        if (idx == sel_index) {
            out->sel_row = i;
        }
    }
}

// 确保选中菜单项在可见范围内，如果不在则调整滚动位置
static void ensure_visible(struct menu_state *st, int visible_rows) {
    // 输入保护
    if (st == NULL || st->current == NULL)
        return;

    int total, sel, max_scroll;
    // 获得菜单项总数和选中菜单项索引
    total = menu_count(st->current);
    sel = menu_index_of(st->current, st->selected);
    if (total <= visible_rows) {
        if (sel < 0)
            sel = 0;
        st->scroll = 0;
        return;
    }
    // 获得选中菜单项索引
    if (sel < 0)
        sel = 0;
    if (sel < st->scroll)
        st->scroll = sel;
    else if (sel >= st->scroll + visible_rows)
        st->scroll = sel - (visible_rows - 1);

    /* clamp scroll 到合法范围 [0, total-visible_rows] */
    max_scroll = total - visible_rows;
    if (st->scroll < 0)
        st->scroll = 0;
    else if (st->scroll > max_scroll)
        st->scroll = max_scroll;
}

//===============save last state================

static void menu_page_save(struct menu_state *st) {
    if (st == NULL || st->current == NULL)
        return;

    st->current->last_sel = st->selected;
    st->current->last_scroll = st->scroll;
}

static void menu_page_restore(struct menu_state *st, int visible_rows) {
    if (st == NULL || st->current == NULL)
        return;

    struct menu *m = st->current;
    st->selected = m->last_sel ? m->last_sel : menu_first_get(m);
    st->scroll = m->last_scroll;

    ensure_visible(st, visible_rows);
}

static void menu_switch_to(struct menu_state *st, struct menu *to,
                           int visible_rows) {
    if (st == NULL || to == NULL || st->current == NULL)
        return;

    menu_page_save(st);
    st->current = to;
    menu_page_restore(st, visible_rows);
}
// 函数实现进入菜单导航状态，设置当前菜单为根菜单，并恢复上次选中菜单项和滚动位置
void menu_system_start(struct menu_system *sys, struct menu *root,
                       int visible_rows) {
    if (sys == NULL || root == NULL)
        return;
    if (visible_rows <= 0)
        visible_rows = MENU_VISIBLE_ROWS;
    g_sys = sys;

    menu_build();
    sys->mode = MENU_MODE_NORMAL;

    sys->navigation.visible_rows = visible_rows;
    sys->navigation.st.current = root;

    menu_page_restore(&sys->navigation.st, visible_rows);
    menu_build_view(&sys->view, &sys->navigation.st, visible_rows);
}
void menu_system_build_view(struct menu_view *out,
                            const struct menu_system *sys) {
    if (!sys)
        return;
    menu_build_view(out, &sys->navigation.st, sys->navigation.visible_rows);
}

const struct menu_view *menu_system_view(const struct menu_system *sys) {
    if (sys == NULL)
        return NULL;
    return &sys->view;
}
//===============handle key================
// 菜单键处理函数，根据按键类型调整选中菜单项和滚动位置
void menu_on_key(struct menu_system *sys, enum menu_key key) {
    struct menu_state *st;
    int visible_rows;
    struct menu_item *first, *last, *sel;

    if (!sys)
        return;

    st = &sys->navigation.st;
    if (!st->current)
        return;

    visible_rows = sys->navigation.visible_rows > 0
                       ? sys->navigation.visible_rows
                       : MENU_VISIBLE_ROWS;

    first = menu_first_get(st->current);
    last = menu_last_get(st->current);
    if (!first || !last)
        return;

    sel = st->selected ? st->selected : first;
    st->selected = sel;

    switch (key) {
    case MENU_KEY_UP:
        if (&sel->node == st->current->items.next)
            st->selected = last;
        else
            st->selected = list_entry(sel->node.prev, struct menu_item, node);
        ensure_visible(st, visible_rows);
        break;

    case MENU_KEY_DOWN:
        if (&sel->node == st->current->items.prev)
            st->selected = first;
        else
            st->selected = list_entry(sel->node.next, struct menu_item, node);
        ensure_visible(st, visible_rows);
        break;

    case MENU_KEY_OK:
        if (sel->type == MENU_ITEM_ACT) {
            if (sel->action)
                sel->action(sel->ctx);
            /* ACT：不切页，位置保持 */
        } else if (sel->type == MENU_ITEM_SUB && sel->child) {
            menu_switch_to(st, sel->child, visible_rows);
        }
        break;

    case MENU_KEY_BACK:
        if (st->current->parent) {
            menu_switch_to(st, st->current->parent, visible_rows);
        }
        break;

    default:
        break;
    }
}

void menu_system_on_key(struct menu_system *sys, int key_value) {
    if (!sys)
        return;
    if (sys->mode == MENU_MODE_NORMAL) {
        menu_on_key(sys, key_value);
        menu_system_build_view(&sys->view, sys);
    } else if (sys->mode == MENU_MODE_EDIT) {
        int done = 0, cancel = 0;
        password_on_key(&sys->pw, key_value, &done, &cancel);
        password_system_build_view(&sys->pv, &sys->pw);
        if (done || cancel) {
            printf("your password:%s\n", sys->pw.save_enter);
            sys->mode = MENU_MODE_NORMAL;
            menu_system_build_view(&sys->view, sys);
        }
    } else if (sys->mode == MENU_MODE_FPS) {
        if (key_value == MENU_KEY_BACK) {
            sys->mode = MENU_MODE_NORMAL;
            menu_system_build_view(&sys->view, sys);
        }
    }
}
