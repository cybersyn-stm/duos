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

/* 番茄图标：32x32，6字节头 + page-major数据（每页32字节×4页） */

static const char gImage_tomato[134] = {
    0X22, 0X01, 0X20, 0X00, 0X20, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
    0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X80, 0X80, 0X00, 0X00,
    0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
    0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X80, 0X60, 0X10,
    0X78, 0X48, 0X34, 0X15, 0X36, 0X40, 0X41, 0X26, 0X14, 0X24, 0X48, 0X78,
    0X10, 0X60, 0X80, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
    0X00, 0X00, 0X00, 0X00, 0X00, 0X0F, 0X30, 0X40, 0X80, 0X80, 0X00, 0X00,
    0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X80, 0X80, 0X40, 0X30, 0X0F, 0X00,
    0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
    0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X01, 0X01, 0X01, 0X01, 0X01, 0X01,
    0X01, 0X01, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
    0X00, 0X00,
};

void pomodoro_init(struct pomodoro_state *pomo);
int pomodoro_fifo_init(int fd);

#endif /* POMODORO_H */
