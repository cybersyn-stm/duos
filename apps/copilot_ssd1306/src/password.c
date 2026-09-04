#include "password.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
void password_enter(struct password_system *ps) {
    if (ps == NULL)
        return;
    ps->editing_len = 0;
    ps->editing[0] = '\0';
}

void password_on_key(struct password_system *ps, int key_value, int *out_done,
                     int *out_cancel) {
    if (ps == NULL)
        return;
    if (key_value >= 0 && key_value <= 9) {
        if (ps->editing_len < MAX_ENTER_WORD) {
            ps->editing[ps->editing_len++] = (char)('0' + key_value);
            ps->editing[ps->editing_len] = '\0';
        }
    }
    switch (key_value) {
    case 14: /* 14 */
        if (ps->editing_len > 0) {
            ps->editing_len--;
            ps->editing[ps->editing_len] = '\0';
        } else {
            if (out_cancel)
                *out_cancel = 1;
        }
        break;

    case 11: /* 11 */
        if (ps->editing_len > 0) {
            /* 保存 */
            memcpy(ps->save_enter, ps->editing, (size_t)ps->editing_len + 1);
            ps->saved_valid = 1;
            if (out_done)
                *out_done = 1;
        }
        break;

    default:
        break;
    }
}

void password_system_build_view(struct password_view *out,
                                const struct password_system *ps) {
    if (out == NULL || ps == NULL)
        return;

    out->title = "Password";
    out->len = ps->editing_len;
    out->saved_valid = ps->saved_valid;

    size_t n = (ps->editing_len <= MAX_ENTER_WORD) ? (size_t)ps->editing_len
                                                   : (size_t)MAX_ENTER_WORD;
    memcpy(out->entered, ps->editing, n);
    out->entered[n] = '\0';
}
