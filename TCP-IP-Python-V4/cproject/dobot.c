/*
 * dobot.c — 越疆机器人 TCP/IP 协议 C 语言实现
 */

#include "dobot.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/* ================================================================
 *  内部工具函数
 * ================================================================ */

static int _tcp_connect(const char *ip, int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    /* 设置 TCP_NODELAY 禁用 Nagle 算法, 保证指令即时发送 */
    int flag = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

    /* 设置接收缓冲区 144000 字节 */
    int rcvbuf = 144000;
    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
        fprintf(stderr, "inet_pton failed for %s\n", ip);
        close(fd);
        return -1;
    }

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "connect to %s:%d failed: %s\n", ip, port,
                strerror(errno));
        close(fd);
        return -1;
    }

    printf("[OK] connected to %s:%d\n", ip, port);
    return fd;
}

/* ================================================================
 *  API 实现
 * ================================================================ */

int dobot_connect_dashboard(const char *ip) { return _tcp_connect(ip, 29999); }

int dobot_connect_feedback(const char *ip) { return _tcp_connect(ip, 30004); }

void dobot_close(int fd) {
    if (fd >= 0) {
        shutdown(fd, SHUT_RDWR);
        close(fd);
    }
}

const char *dobot_send_cmd(int fd, const char *cmd) {
    static char reply_buf[4096];

    /* 发送指令 */
    size_t len = strlen(cmd);
    ssize_t sent = send(fd, cmd, len, 0);
    if (sent < 0) {
        perror("send");
        return NULL;
    }
    printf("[SEND] %s\n", cmd);

    /* 读取回复 */
    memset(reply_buf, 0, sizeof(reply_buf));
    ssize_t n = recv(fd, reply_buf, sizeof(reply_buf) - 1, 0);
    if (n <= 0) {
        if (n == 0)
            fprintf(stderr, "connection closed by robot\n");
        else
            perror("recv");
        return NULL;
    }

    reply_buf[n] = '\0';
    printf("[RECV] %s\n", reply_buf);
    return reply_buf;
}

int dobot_read_feedback(int fd, FeedbackPacket *data) {
    uint8_t *buf = (uint8_t *)data;
    size_t total = 0;

    while (total < 1440) {
        ssize_t n = recv(fd, buf + total, 1440 - total, 0);
        if (n <= 0) {
            if (n == 0)
                fprintf(stderr, "feedback connection closed\n");
            else
                perror("recv feedback");
            return -1;
        }
        total += n;
    }

    /* 丢弃多余数据 (反馈端口持续推送) */
    /* 简单策略: 再读一次清空缓冲区, 拿走最新一帧 */
    return 0;
}

/* ================================================================
 *  便捷指令封装 — 格式与 Python 端完全一致
 * ================================================================ */

const char *dobot_EnableRobot(int fd) {
    return dobot_send_cmd(fd, "EnableRobot()");
}

const char *dobot_DisableRobot(int fd) {
    return dobot_send_cmd(fd, "DisableRobot()");
}

const char *dobot_ClearError(int fd) {
    return dobot_send_cmd(fd, "ClearError()");
}

const char *dobot_Stop(int fd) { return dobot_send_cmd(fd, "Stop()"); }

const char *dobot_RobotMode(int fd) {
    return dobot_send_cmd(fd, "RobotMode()");
}

const char *dobot_PowerOn(int fd) { return dobot_send_cmd(fd, "PowerOn()"); }

const char *dobot_SpeedFactor(int fd, int percent) {
    static char cmd[64];
    snprintf(cmd, sizeof(cmd), "SpeedFactor(%d)", percent);
    return dobot_send_cmd(fd, cmd);
}

const char *dobot_StartDrag(int fd) {
    return dobot_send_cmd(fd, "StartDrag()");
}

const char *dobot_StopDrag(int fd) { return dobot_send_cmd(fd, "StopDrag()"); }

const char *dobot_ServoJ(int fd, double j1, double j2, double j3, double j4,
                         double j5, double j6, double t, double gain) {
    static char cmd[512];
    int pos = snprintf(cmd, sizeof(cmd), "ServoJ(%f,%f,%f,%f,%f,%f", j1, j2, j3,
                       j4, j5, j6);
    if (t > 0)
        pos += snprintf(cmd + pos, sizeof(cmd) - pos, ",t=%f", t);
    if (gain > 0)
        pos += snprintf(cmd + pos, sizeof(cmd) - pos, ",gain=%f", gain);
    snprintf(cmd + pos, sizeof(cmd) - pos, ")");
    return dobot_send_cmd(fd, cmd);
}

const char *dobot_MovJ_pose(int fd, double x, double y, double z, double rx,
                            double ry, double rz) {
    static char cmd[512];
    snprintf(cmd, sizeof(cmd), "MovJ(pose={%f,%f,%f,%f,%f,%f},0)", x, y, z, rx,
             ry, rz);
    return dobot_send_cmd(fd, cmd);
}

const char *dobot_MovL_pose(int fd, double x, double y, double z, double rx,
                            double ry, double rz) {
    static char cmd[512];
    snprintf(cmd, sizeof(cmd), "MovL(pose={%f,%f,%f,%f,%f,%f},0)", x, y, z, rx,
             ry, rz);
    return dobot_send_cmd(fd, cmd);
}

const char *dobot_GetAngle(int fd) { return dobot_send_cmd(fd, "GetAngle()"); }

const char *dobot_GetPose(int fd) { return dobot_send_cmd(fd, "GetPose()"); }
