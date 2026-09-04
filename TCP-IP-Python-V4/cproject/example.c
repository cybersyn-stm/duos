/*
 * example.c — 越疆机器人 C 语言控制示例
 *
 * 编译:
 *   gcc -o example example.c dobot.c
 * 运行:
 *   ./example 192.168.0.32
 *
 * 依赖: 无第三方库, 仅标准 C + POSIX socket
 */

#include "dobot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     // sleep

/* 解析返回值: "0,1234,RobotMode();" -> error_id=0, result_id=1234 */
static int parse_result(const char *reply, int *out_result_id)
{
    if (!reply) {
        fprintf(stderr, "[ERROR] no reply\n");
        return -1;
    }
    if (strstr(reply, "Not Tcp")) {
        fprintf(stderr, "[ERROR] Control Mode Is Not Tcp\n");
        return -1;
    }
    int err, rid;
    if (sscanf(reply, "%d,%d", &err, &rid) >= 2) {
        if (out_result_id) *out_result_id = rid;
        return err;
    }
    /* 可能只有 ErrorID */
    if (sscanf(reply, "%d", &err) == 1) {
        return err;
    }
    return 2;  /* 解析失败 */
}

/* RobotMode 编码 -> 名称 */
static const char *mode_name(uint64_t mode)
{
    switch (mode) {
        case 1:  return "INIT";
        case 2:  return "BRAKE_OPEN";
        case 3:  return "POWEROFF";
        case 4:  return "DISABLED";
        case 5:  return "ENABLE_IDLE";
        case 6:  return "BACKDRIVE";
        case 7:  return "RUNNING";
        case 8:  return "SINGLE_MOVE";
        case 9:  return "ERROR";
        case 10: return "PAUSE";
        case 11: return "COLLISION";
        default: return "UNKNOWN";
    }
}

int main(int argc, char *argv[])
{
    const char *ip = (argc > 1) ? argv[1] : "192.168.0.32";

    printf("========================================\n");
    printf("  Dobot C Demo — Target IP: %s\n", ip);
    printf("========================================\n\n");

    /* ---- 1. 连接 ---- */
    printf("[1/4] Connecting to dashboard (port 29999)...\n");
    int dash_fd = dobot_connect_dashboard(ip);
    if (dash_fd < 0) return 1;

    printf("[1/4] Connecting to feedback (port 30004)...\n");
    int feed_fd = dobot_connect_feedback(ip);
    if (feed_fd < 0) { dobot_close(dash_fd); return 1; }

    /* ---- 2. 使能 ---- */
    printf("\n[2/4] Enabling robot...\n");
    const char *reply = dobot_EnableRobot(dash_fd);
    if (parse_result(reply, NULL) != 0) {
        fprintf(stderr, "Enable failed, check if port 29999 is occupied\n");
        dobot_close(dash_fd);
        dobot_close(feed_fd);
        return 1;
    }
    printf("Enable OK\n");

    /* ---- 3. 读取反馈 ---- */
    printf("\n[3/4] Reading feedback in a loop (Ctrl+C to stop)...\n");
    FeedbackPacket fb;
    int count = 0;

    while (count < 20) {   /* 演示 20 帧 */
        if (dobot_read_feedback(feed_fd, &fb) < 0) break;

        printf("--- Frame %d ---\n", ++count);
        printf("  RobotMode     : %llu (%s)\n",
               (unsigned long long)fb.RobotMode, mode_name(fb.RobotMode));
        printf("  DigitalInputs : 0x%llX\n",
               (unsigned long long)fb.DigitalInputs);
        printf("  DigitalOutputs: 0x%llX\n",
               (unsigned long long)fb.DigitalOutputs);
        printf("  CommandID     : %llu\n",
               (unsigned long long)fb.CurrentCommandId);
        printf("  QActual (deg) : %.2f %.2f %.2f %.2f %.2f %.2f\n",
               fb.QActual[0], fb.QActual[1], fb.QActual[2],
               fb.QActual[3], fb.QActual[4], fb.QActual[5]);
        printf("  TestValue     : 0x%llX (expect 0x123456789abcdef)\n",
               (unsigned long long)fb.TestValue);

        sleep(1);
    }

    /* ---- 4. 查询位姿 ---- */
    printf("\n[4/4] Querying current pose...\n");
    reply = dobot_GetPose(dash_fd);
    if (reply) {
        double x, y, z, rx, ry, rz;
        int err;
        if (sscanf(reply, "%d,%lf,%lf,%lf,%lf,%lf,%lf",
                   &err, &x, &y, &z, &rx, &ry, &rz) >= 7) {
            printf("  Pose: X=%.2f Y=%.2f Z=%.2f Rx=%.2f Ry=%.2f Rz=%.2f\n",
                   x, y, z, rx, ry, rz);
        }
    }

    /* ---- 清理 ---- */
    dobot_close(dash_fd);
    dobot_close(feed_fd);
    printf("\nDone.\n");
    return 0;
}
