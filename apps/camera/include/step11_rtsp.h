#ifndef STEP11_RTSP_H
#define STEP11_RTSP_H

#include <stdint.h>
#include <pthread.h>

#include <cvi_common.h>
#include <cvi_rtsp/rtsp.h>

CVI_S32 step11_rtsp_create(CVI_RTSP_CTX **ctx, CVI_RTSP_SESSION **session, const char *codec_name);
CVI_S32 step11_rtsp_start(CVI_RTSP_CTX *ctx, CVI_RTSP_SESSION *session);
CVI_S32 step11_rtsp_stop(CVI_RTSP_CTX *ctx, CVI_RTSP_SESSION *session);

#endif
