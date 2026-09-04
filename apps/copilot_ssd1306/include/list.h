#ifndef LIST_H
#define LIST_H

#include <stddef.h>

/*
 * Intrusive doubly-linked circular list (Linux-kernel style).
 *
 * Usage:
 *   struct foo {
 *     int x;
 *     struct list_head node;
 *   };
 *
 *   LIST_HEAD(head);
 *   struct foo a;
 *   INIT_LIST_HEAD(&a.node);
 *   list_add_tail(&a.node, &head);
 */

struct list_head {
    struct list_head *next;
    struct list_head *prev;
};

/* static list head initializer */
#define LIST_HEAD_INIT(name) {&(name), &(name)}

/* define and init a list head */
#define LIST_HEAD(name) struct list_head name = LIST_HEAD_INIT(name)

/* init a list head at runtime */
static inline void INIT_LIST_HEAD(struct list_head *list) {
    list->next = list;
    list->prev = list;
}

static inline int list_empty(const struct list_head *head) {
    return head->next == head;
}

static inline void __list_add(struct list_head *new_node,
                              struct list_head *prev, struct list_head *next) {
    next->prev = new_node;
    new_node->next = next;
    new_node->prev = prev;
    prev->next = new_node;
}

static inline void __list_del(struct list_head *prev, struct list_head *next) {
    next->prev = prev;
    prev->next = next;
}

static inline void list_add_head(struct list_head *new_node,
                                 struct list_head *head) {
    /* add after head (stack behavior) */
    __list_add(new_node, head, head->next);
}

static inline void list_add_tail(struct list_head *new_node,
                                 struct list_head *head) {
    /* add before head (queue behavior) */
    __list_add(new_node, head->prev, head);
}

static inline void list_del(struct list_head *entry) {
    __list_del(entry->prev, entry->next);
    /* poison to help catch bugs */
    entry->next = NULL;
    entry->prev = NULL;
}

static inline void list_del_init(struct list_head *entry) {
    __list_del(entry->prev, entry->next);
    INIT_LIST_HEAD(entry);
}

static inline void list_move(struct list_head *entry, struct list_head *head) {
    __list_del(entry->prev, entry->next);
    list_add_head(entry, head);
}

static inline void list_move_tail(struct list_head *entry,
                                  struct list_head *head) {
    __list_del(entry->prev, entry->next);
    list_add_tail(entry, head);
}

#define offsetof_member(TYPE, MEMBER) ((size_t)&(((TYPE *)0)->MEMBER))

#define container_of(ptr, type, member)                                        \
    ((type *)((char *)(ptr) - offsetof_member(type, member)))

#define list_entry(ptr, type, member) container_of(ptr, type, member)

/* iterate over list nodes */
#define list_for_each(pos, head)                                               \
    for ((pos) = (head)->next; (pos) != (head); (pos) = (pos)->next)

#define list_for_each_prev(pos, head)                                          \
    for ((pos) = (head)->prev; (pos) != (head); (pos) = (pos)->prev)

/* safe iteration (allows deletion during loop) */
#define list_for_each_safe(pos, n, head)                                       \
    for ((pos) = (head)->next, (n) = (pos)->next; (pos) != (head);             \
         (pos) = (n), (n) = (pos)->next)

#endif /* LIST_H */
