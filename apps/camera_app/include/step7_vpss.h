/**
 * Step 7: VPSS 双通道初始化
 *
 * CHN0: 640x480 NV12 — 拍照快照通道
 * CHN1: 1920x1080 YUV_PLANAR_420 — VENC H.264 编码通道
 */

#ifndef STEP7_VPSS_H
#define STEP7_VPSS_H

#include "linux/cvi_type.h"
#include "linux/cvi_common.h"
#include "linux/cvi_comm_vpss.h"

/* VPSS 参数 */
#define VPSS_GRP_NUM    0
#define VPSS_CHN_SNAP   0   /* 快照通道: NV12 */
#define VPSS_CHN_STREAM 1   /* 编码通道: YUV_PLANAR_420 */
#define VPSS_IN_W       1920
#define VPSS_IN_H       1080
#define VPSS_SNAP_W     640
#define VPSS_SNAP_H     480
#define VPSS_STREAM_W   1920
#define VPSS_STREAM_H   1080

/**
 * 初始化 VPSS 双通道
 * @param VpssGrp  VPSS Group 编号
 * @return 0 成功，-1 失败
 */
int step7_vpss_init(VPSS_GRP VpssGrp);

/**
 * 启动 VPSS Group（设置通道属性后调用）
 * @param VpssGrp  VPSS Group 编号
 * @return 0 成功，-1 失败
 */
int step7b_vpss_start(VPSS_GRP VpssGrp);

/**
 * 绑定 VI → VPSS
 * @return 0 成功，-1 失败
 */
int step7c_bind_vi_to_vpss(void);

/**
 * 清理 VPSS
 * @param VpssGrp  VPSS Group 编号
 */
void step7_vpss_cleanup(VPSS_GRP VpssGrp);

#endif /* STEP7_VPSS_H */
