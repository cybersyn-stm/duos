#ifndef PASSWORD_H
#define PASSWORD_H

#define MAX_ENTER_WORD 6

struct password_view {
    const char *title;
    char entered[MAX_ENTER_WORD + 1];
    int len;
    int saved_valid;
};
struct password_system {
    char editing[MAX_ENTER_WORD + 1];
    int editing_len;

    char save_enter[MAX_ENTER_WORD + 1];
    int saved_valid;
};
void password_enter(struct password_system *ps);
void password_on_key(struct password_system *ps, int key_value, int *out_done,
                     int *out_cancel);
void password_system_build_view(struct password_view *out,
                                const struct password_system *ps);
#endif
