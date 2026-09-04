#ifndef __LINKLISTED_H__
#define __LINKLISTED_H__

typedef struct MenuCtx_t MenuCtx_t;
typedef struct linklisted {
    char *name;
    void (*function)(MenuCtx_t *);
    struct linklisted *next;
    struct linklisted *prev;
    struct linklisted *sublist;
    struct linklisted *parentlist;
} linklisted_t;
linklisted_t *linklisted_create(char *name, void (*function)(MenuCtx_t *),
                                linklisted_t *sublist);
linklisted_t *linklisted_add(linklisted_t *head, linklisted_t *node);
linklisted_t *linklisted_slide(linklisted_t *head, int step);
#endif // !#ifndef __LINKLISTED_H__
