/**
 * Step 11: RTSP 流媒体服务
 *
 * URL: rtsp://<ip>/h264  端口: 554
 * 基于 live555 的 cvi_rtsp 库
 */

#ifndef STEP11_RTSP_H
#define STEP11_RTSP_H

#include <stdint.h>
#include <pthread.h>
#include "cvi_rtsp/rtsp.h"

/**
 * 创建 RTSP 服务并注册 h264 会话
 * @return 0 成功，-1 失败。
 *         成功后将 ctx / session 存入全局变量。
 */
int step11_rtsp_create(void);

/**
 * 启动 RTSP 服务（开始监听端口 554）
 * @return 0 成功，-1 失败
 */
int step11_rtsp_start(void);

/**
 * 停止并销毁 RTSP 服务
 */
void step11_rtsp_stop(void);

/**
 * 获取全局 RTSP 上下文（供流传输线程使用）
 */
CVI_RTSP_CTX *step11_rtsp_get_ctx(void);

/**
 * 获取全局 RTSP 会话（供流传输线程使用）
 */
CVI_RTSP_SESSION *step11_rtsp_get_session(void);

#endif /* STEP11_RTSP_H */
