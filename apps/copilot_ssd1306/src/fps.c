#include "fps.h"
#include <stdint.h>
#include <time.h>

static uint64_t ms_diff(uint64_t a, uint64_t b) {
    return (a >= b) ? (a - b) : 0;
}

void fps_init(struct fps_system *c, uint64_t now_ms) {
    if (!c)
        return;
    c->frames_in_window = 0;
    c->last_fps = 0;
    c->window_start_ms = now_ms;
    c->box_col = 0;
}

void fps_on_frame(struct fps_system *c, uint64_t now_ms) {
    if (!c)
        return;

    c->frames_in_window++;

    c->box_col++;
    if (c->box_col >= FPS_BAR_COLS)
        c->box_col = 0;

    uint64_t elapsed =
        (now_ms >= c->window_start_ms) ? (now_ms - c->window_start_ms) : 0;
    if (elapsed >= 1000) {
        c->last_fps = c->frames_in_window; // 记录fps
        c->frames_in_window = 0;           // 重置帧数计数器
        c->window_start_ms = now_ms;       // 重置窗口起始时间
    }
}

uint64_t now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
}
