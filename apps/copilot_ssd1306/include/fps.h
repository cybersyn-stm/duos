#ifndef FPS_H
#define FPS_H

#include <stdint.h>
#define FPS_BAR_COLS 16

struct fps_system {
    uint32_t frames_in_window;
    uint32_t last_fps;

    uint64_t window_start_ms;

    uint8_t box_col;
};

void fps_init(struct fps_system *c, uint64_t now_ms);
void fps_on_frame(struct fps_system *c, uint64_t now_ms);
uint64_t now_ms(void);
#endif
