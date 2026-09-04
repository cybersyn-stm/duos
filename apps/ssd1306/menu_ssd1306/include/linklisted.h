#ifndef __LINKLISTED_H__
#define __LINKLISTED_H__

typedef struct linklisted {
    char *name;
    void (*function)(void);
    struct linklisted *next;
    struct linklisted *prev;
} linklisted_t;
linklisted_t *linkedlist_create(char *name, void (*function)(void));
linklisted_t *linklisted_add(linklisted_t *head, char *name,
                             void (*function)(void));
linklisted_t *linklisted_slide(linklisted_t *head, int step);
#endif // !#ifndef __LINKLISTED_H__
