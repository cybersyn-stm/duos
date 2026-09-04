/**
 * Step 11: RTSP 流媒体实现
 *
 * URL: rtsp://<ip>/h264  端口: 554
 * 回调: 打印客户端连接/断开日志
 */

#include <stdio.h>
#include <string.h>
#include "step11_rtsp.h"

/* 全局 RTSP 上下文 */
static CVI_RTSP_CTX     *g_rtspCtx;
static CVI_RTSP_SESSION *g_rtspSession;

/* ─── 连接/断开回调 ─── */
static void on_connect(const char *ip, void *arg)
{
    (void)arg;
    printf("[RTSP] 客户端连接: %s\n", ip);
}

static void on_disconnect(const char *ip, void *arg)
{
    (void)arg;
    printf("[RTSP] 客户端断开: %s\n", ip);
}

/* ─── 公开接口 ─── */
int step11_rtsp_create(void)
{
    CVI_RTSP_CONFIG config;
    CVI_RTSP_SESSION_ATTR attr;
    CVI_RTSP_STATE_LISTENER listener;
    int ret;

    printf("[RTSP] 创建 RTSP 服务 (port=554, session=h264)...\n");

    /* 1. 创建 RTSP 上下文 */
    memset(&config, 0, sizeof(config));
    config.port         = 554;
    config.maxConnNum   = 4;
    config.timeout      = 30;

    ret = CVI_RTSP_Create(&g_rtspCtx, &config);
    if (ret != 0) {
        fprintf(stderr, "[ERROR] CVI_RTSP_Create 失败\n");
        return -1;
    }

    /* 2. 创建 h264 会话 */
    memset(&attr, 0, sizeof(attr));
    attr.video.codec = RTSP_VIDEO_H264;
    attr.video.bitrate = 1800;
    snprintf(attr.name, sizeof(attr.name), "h264");

    ret = CVI_RTSP_CreateSession(g_rtspCtx, &attr, &g_rtspSession);
    if (ret != 0) {
        fprintf(stderr, "[ERROR] CVI_RTSP_CreateSession 失败\n");
        CVI_RTSP_Destroy(&g_rtspCtx);
        g_rtspCtx = NULL;
        return -1;
    }

    /* 3. 设置连接/断开监听 */
    memset(&listener, 0, sizeof(listener));
    listener.onConnect    = on_connect;
    listener.argConn      = g_rtspCtx;
    listener.onDisconnect = on_disconnect;
    listener.argDisconn   = g_rtspCtx;

    CVI_RTSP_SetListener(g_rtspCtx, &listener);

    printf("[RTSP] RTSP 服务创建完成 (URL: rtsp://<ip>/h264)\n");
    return 0;
}

int step11_rtsp_start(void)
{
    if (!g_rtspCtx) {
        fprintf(stderr, "[ERROR] RTSP 上下文未创建\n");
        return -1;
    }

    printf("[RTSP] 启动监听端口 554...\n");

    if (CVI_RTSP_Start(g_rtspCtx) != 0) {
        fprintf(stderr, "[ERROR] CVI_RTSP_Start 失败\n");
        return -1;
    }

    printf("[RTSP] 流媒体服务已启动\n");
    return 0;
}

void step11_rtsp_stop(void)
{
    if (!g_rtspCtx)
        return;

    CVI_RTSP_Stop(g_rtspCtx);
    CVI_RTSP_DestroySession(g_rtspCtx, g_rtspSession);
    CVI_RTSP_Destroy(&g_rtspCtx);
    g_rtspCtx     = NULL;
    g_rtspSession = NULL;
    printf("[RTSP] 已停止\n");
}

CVI_RTSP_CTX *step11_rtsp_get_ctx(void)
{
    return g_rtspCtx;
}

CVI_RTSP_SESSION *step11_rtsp_get_session(void)
{
    return g_rtspSession;
}
