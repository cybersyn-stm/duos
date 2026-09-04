#include "step11_rtsp.h"

#include <stdio.h>
#include <string.h>

static void _on_connect(const char *ip, void *arg) {
    (void)arg;
    printf("[RTSP] Client connected: %s\n", ip);
}

static void _on_disconnect(const char *ip, void *arg) {
    (void)arg;
    printf("[RTSP] Client disconnected: %s\n", ip);
}

CVI_S32 step11_rtsp_create(CVI_RTSP_CTX **ctx, CVI_RTSP_SESSION **session, const char *codec_name) {
    CVI_S32 ret;

    CVI_RTSP_CONFIG config = {0};
    config.port = 554;

    ret = CVI_RTSP_Create(ctx, &config);
    if (ret != 0) {
        printf("[RTSP] Create failed: %d\n", ret);
        return CVI_FAILURE;
    }

    CVI_RTSP_SESSION_ATTR attr = {0};
    if (strcmp(codec_name, "h265") == 0)
        attr.video.codec = RTSP_VIDEO_H265;
    else
        attr.video.codec = RTSP_VIDEO_H264;
    snprintf(attr.name, sizeof(attr.name), "%s", codec_name);

    ret = CVI_RTSP_CreateSession(*ctx, &attr, session);
    if (ret != 0) {
        printf("[RTSP] CreateSession failed: %d\n", ret);
        return CVI_FAILURE;
    }

    CVI_RTSP_STATE_LISTENER listener = {0};
    listener.onConnect = _on_connect;
    listener.argConn = *ctx;
    listener.onDisconnect = _on_disconnect;
    listener.argDisconn = *ctx;
    CVI_RTSP_SetListener(*ctx, &listener);

    printf("[RTSP] Session '%s' created (port 554)\n", codec_name);
    return CVI_SUCCESS;
}

CVI_S32 step11_rtsp_start(CVI_RTSP_CTX *ctx, CVI_RTSP_SESSION *session) {
    (void)session;
    CVI_S32 ret;

    ret = CVI_RTSP_Start(ctx);
    if (ret != 0) {
        printf("[RTSP] Start failed: %d\n", ret);
        return CVI_FAILURE;
    }
    printf("[RTSP] Streaming - rtsp://<ip>/h264\n");
    return CVI_SUCCESS;
}

CVI_S32 step11_rtsp_stop(CVI_RTSP_CTX *ctx, CVI_RTSP_SESSION *session) {
    CVI_RTSP_Stop(ctx);
    CVI_RTSP_DestroySession(ctx, session);
    CVI_RTSP_Destroy(&ctx);
    printf("[RTSP] Stopped\n");
    return CVI_SUCCESS;
}
