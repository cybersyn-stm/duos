/*
 * sync_arm.c — 双机械臂拖动跟随 + 录制回放 (C 复现 learn_start.py)
 *
 * 编译: make sync_arm   或   gcc -o sync_arm sync_arm.c dobot.c
 *
 * 用法: ./sync_arm [master_ip] [follower_ip]
 *       默认: master=192.168.0.34  follower=192.168.0.32
 *
 * 运行时输入 "START" 回车 → 停止录制并回放
 */

#include "dobot.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

/* ================================================================
 *  可调参数 (对应 Python 端全局配置)
 * ================================================================ */
#define SPEED_PERCENT 30
#define SERVO_GAIN 200.0      /* 回放增益 (平滑) */
#define FOLLOW_GAIN 500.0     /* 跟随增益 (灵敏) */
#define RECORD_HZ 1           /* 录制采样率 */
#define PLAYBACK_INTERVAL 1.0 /* 回放每步间隔(秒) */
#define FOLLOW_T 0.05         /* ServoJ 跟随时间 */
#define MAX_PATH_POINTS 10000 /* 最大录制点数 */

/* 路径点 */
typedef struct {
    double j1, j2, j3, j4, j5, j6;
} JointPoint;

static JointPoint path[MAX_PATH_POINTS];
static int path_count = 0;

/* ================================================================
 *  工具函数
 * ================================================================ */

/* 解析返回值: "0,1.23,4.56,..." -> error_id, 并把后续数字填入 out (最多 n 个)
 */
static int parse_numbers(const char *reply, double *out, int n) {
    if (!reply)
        return -1;
    const char *p = reply;
    int count = 0;
    while (*p && count < n) {
        /* 跳过非数字非负号字符 */
        while (*p && *p != '-' && (*p < '0' || *p > '9'))
            p++;
        if (!*p)
            break;

        char *end;
        out[count++] = strtod(p, &end);
        p = end;
    }
    return count;
}

/* 获取 RobotMode */
static int get_robot_mode(int fd) {
    const char *r = dobot_RobotMode(fd);
    double nums[4];
    int n = parse_numbers(r, nums, 4);
    return (n >= 2) ? (int)nums[1] : -1;
}

static const char *mode_str(int mode) {
    switch (mode) {
    case 4:
        return "断电";
    case 5:
        return "使能空闲";
    case 6:
        return "拖动模式";
    case 7:
        return "运行中";
    case 9:
        return "报警";
    default:
        return "未知";
    }
}

/* 检查状态并按需使能 (完整复现 check_and_enable) */
static int check_and_enable(int fd, const char *name) {
    int mode = get_robot_mode(fd);
    printf("  [%s] 当前状态: RobotMode=%d (%s)\n", name, mode, mode_str(mode));

    if (mode == 5) {
        dobot_ClearError(fd);
        usleep(300000);
        printf("  OK [%s] 已就绪，无需额外操作\n", name);
        return 1;
    }
    if (mode == 9) {
        printf("  [%s] 清除报警...\n", name);
        dobot_ClearError(fd);
        usleep(500000);
    }
    if (mode == 4 || mode <= 0) {
        printf("  [%s] 上电中...\n", name);
        dobot_PowerOn(fd);
        sleep(10);
        printf("  [%s] 清除报警...\n", name);
        dobot_ClearError(fd);
        usleep(500000);
    }

    printf("  [%s] 停止运动...\n", name);
    dobot_Stop(fd);
    usleep(500000);

    printf("  [%s] 使能中...\n", name);
    const char *r = dobot_EnableRobot(fd);
    double nums[4];
    int n = parse_numbers(r, nums, 4);
    if (n < 1 || (int)nums[0] != 0) {
        printf("  FAIL [%s] 使能失败! 返回值: %s\n", name, r ? r : "(null)");
        return 0;
    }
    printf("  OK [%s] 使能成功\n", name);
    return 1;
}

/* 非阻塞检查 stdin 是否有输入 (返回 1=有数据) */
static int stdin_has_input(void) {
    fd_set set;
    struct timeval tv = {0, 0}; /* 立即返回, 不阻塞 */
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);
    return select(STDIN_FILENO + 1, &set, NULL, NULL, &tv) > 0;
}

/* 读取一行 (简单实现, 去掉尾部换行) */
static int read_line(char *buf, int size) {
    if (!fgets(buf, size, stdin))
        return -1;
    int len = strlen(buf);
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
        buf[--len] = '\0';
    return len;
}

/* 高精度时间 (秒) */
static double now_sec(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec * 1e-6;
}

/* ================================================================
 *  主流程
 * ================================================================ */
int main(int argc, char *argv[]) {
    const char *master_ip = (argc > 1) ? argv[1] : "192.168.0.34";
    const char *follower_ip = (argc > 2) ? argv[2] : "192.168.0.32";

    /* ---- 安全确认 ---- */
    printf("==================================================\n");
    printf("  警告：本程序将检测机械臂状态并按需启动，进入拖动跟随模式\n");
    printf("==================================================\n");
    printf("  Master  : %s\n", master_ip);
    printf("  Follower: %s\n", follower_ip);
    printf("  速度限制: %d%%\n", SPEED_PERCENT);
    printf("  ServoJ 跟随增益: %.0f (回放增益: %.0f)\n", FOLLOW_GAIN,
           SERVO_GAIN);
    printf("  录制采样: %d Hz\n", RECORD_HZ);
    printf("  回放间隔: %.1fs/步\n", PLAYBACK_INTERVAL);
    printf("  输入 START 回车 → 停止录制并回放路径\n");
    printf("==================================================\n");
    printf("按 Enter 继续，Ctrl+C 退出...");
    fflush(stdout);
    getchar(); /* 等待回车 */

    /* ---- 第 1 步：连接 ---- */
    printf("\n[1/5] 连接 Master %s:29999 ...\n", master_ip);
    int master_fd = dobot_connect_dashboard(master_ip);
    if (master_fd < 0)
        return 1;

    printf("[1/5] 连接 Follower %s:29999 ...\n", follower_ip);
    int follower_fd = dobot_connect_dashboard(follower_ip);
    if (follower_fd < 0) {
        dobot_close(master_fd);
        return 1;
    }

    /* ---- 第 2 步：检测状态并按需使能 ---- */
    printf("\n[2/5] 检测机器人状态...\n");
    if (!check_and_enable(master_fd, "Master")) {
        dobot_close(master_fd);
        dobot_close(follower_fd);
        return 1;
    }
    if (!check_and_enable(follower_fd, "Follower")) {
        dobot_close(master_fd);
        dobot_close(follower_fd);
        return 1;
    }

    /* ---- 第 3 步：限速 ---- */
    printf("\n[3/5] 设置全局速度 %d%% ...\n", SPEED_PERCENT);
    dobot_SpeedFactor(master_fd, SPEED_PERCENT);
    dobot_SpeedFactor(follower_fd, SPEED_PERCENT);
    printf("  OK 速度限制已生效\n");

    /* ---- 第 4 步：拖动跟随 + 录制 ---- */
    printf("\n[4/5] Master 进入拖动模式，开始录制...\n");
    printf("       Follower 实时跟随 (gain=%.0f)\n", FOLLOW_GAIN);
    printf("       录制采样 %d Hz | 输入 START 回车触发回放\n\n", RECORD_HZ);
    dobot_StartDrag(master_fd);

    double record_interval = 1.0 / RECORD_HZ;
    double last_record = now_sec();
    int playback = 0;
    path_count = 0;

    while (!playback) {
        /* 检查键盘输入 */
        if (stdin_has_input()) {
            char line[256];
            if (read_line(line, sizeof(line)) >= 0) {
                if (strcasecmp(line, "START") == 0) {
                    playback = 1;
                    break;
                }
            }
        }

        /* 获取 Master 关节角度 */
        const char *reply = dobot_GetAngle(master_fd);
        double nums[8];
        int n = parse_numbers(reply, nums, 8);
        if (n < 7) {
            usleep(10000);
            continue;
        }
        double j1 = nums[1], j2 = nums[2], j3 = nums[3];
        double j4 = nums[4], j5 = nums[5], j6 = nums[6];

        /* Follower 实时跟随 */
        dobot_ServoJ(follower_fd, j1, j2, j3, j4, j5, j6, FOLLOW_T,
                     FOLLOW_GAIN);

        /* 按采样频率录制 */
        double t = now_sec();
        if (t - last_record >= record_interval &&
            path_count < MAX_PATH_POINTS) {
            path[path_count].j1 = j1;
            path[path_count].j2 = j2;
            path[path_count].j3 = j3;
            path[path_count].j4 = j4;
            path[path_count].j5 = j5;
            path[path_count].j6 = j6;
            path_count++;
            last_record = t;
        }

        printf("\r  录制中 [%d] J1=%7.2f J2=%7.2f J3=%7.2f J4=%7.2f J5=%7.2f "
               "J6=%7.2f",
               path_count, j1, j2, j3, j4, j5, j6);
        fflush(stdout);
        usleep(10000); /* 10ms 循环 */
    }

    /* ---- 第 5 步：停止拖动，回放路径 ---- */
    printf("\n\n[5/5] 停止拖动，准备回放 %d 个路径点...\n", path_count);
    dobot_StopDrag(master_fd);
    usleep(500000);

    if (path_count == 0) {
        printf("  没有录制到路径点，退出\n");
        goto cleanup;
    }

    printf("开始回放（间隔 %.1fs，共 %d 步）...\n\n", PLAYBACK_INTERVAL,
           path_count);
    for (int i = 0; i < path_count; i++) {
        JointPoint *p = &path[i];
        dobot_ServoJ(master_fd, p->j1, p->j2, p->j3, p->j4, p->j5, p->j6,
                     PLAYBACK_INTERVAL, SERVO_GAIN);
        dobot_ServoJ(follower_fd, p->j1, p->j2, p->j3, p->j4, p->j5, p->j6,
                     PLAYBACK_INTERVAL, SERVO_GAIN);

        printf("\r  回放 [%d/%d] J1=%7.2f J2=%7.2f J3=%7.2f J4=%7.2f J5=%7.2f "
               "J6=%7.2f",
               i + 1, path_count, p->j1, p->j2, p->j3, p->j4, p->j5, p->j6);
        fflush(stdout);
        usleep((int)(PLAYBACK_INTERVAL * 1000000));
    }

    printf("\n\n回放完成!\n");

cleanup:
    dobot_close(master_fd);
    dobot_close(follower_fd);
    return 0;
}
