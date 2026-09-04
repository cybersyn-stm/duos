#ifndef POMODORO_H
#define POMODORO_H

#include <stdint.h>
#include <time.h>

/* 显示模式 */
enum display_mode {
    MODE_CLOCK = 0,
    MODE_POMODORO = 1,
};

/* 番茄钟阶段 */
enum pomo_phase {
    POMO_WORK = 0,
    POMO_SHORT_BREAK = 1,
    POMO_LONG_BREAK = 2,
};

/* 各阶段默认时长（秒） */
#define POMO_WORK_SEC (25 * 60)
#define POMO_SHORT_BREAK_SEC (5 * 60)
#define POMO_LONG_BREAK_SEC (15 * 60)
#define POMO_LONG_BREAK_EVERY 4

/* FIFO 命令管道 */
#define POMO_FIFO_PATH "/tmp/ds3231_cmd"

/* 番茄钟运行状态 */
struct pomodoro_state {
    int mode;
    int phase;
    int paused;
    int round_count;
    int phase_duration;
    time_t phase_start;
    time_t pause_start;
    int remaining_at_pause;
};

/* 番茄图标：16x16 位图，每行一个 uint16_t，bit15=最左 */
static const uint16_t tomato_icon[16] = {
    0x03E0, 0x0FF8, 0x1FFC, 0x3FFE, 0x3FFE, 0x7FFE, 0x7FFF, 0x7FFF,
    0x3FFE, 0x3FFE, 0x1FFC, 0x0FF8, 0x07F0, 0x03E0, 0x0180, 0x0180,
};

void pomodoro_init(struct pomodoro_state *pomo);
int pomodoro_fifo_init(int fd);

#endif /* POMODORO_H */
