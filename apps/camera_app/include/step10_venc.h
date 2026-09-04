/**
 * Step 10: VENC H.264 编码器初始化
 *
 * 配置: H.264 Baseline, CBR 1800kbps, GOP60, 1080p@30fps
 */

#ifndef STEP10_VENC_H
#define STEP10_VENC_H

#include "linux/cvi_type.h"
#include "linux/cvi_common.h"

#define VENC_CHN_NUM    0

/**
 * 初始化 VENC H.264 编码通道
 * @param VeChn  VENC 通道编号
 * @return 0 成功，-1 失败
 */
int step10_venc_init(VENC_CHN VeChn);

/**
 * 绑定 VPSS CHN1 → VENC
 * @param VeChn  VENC 通道编号
 * @return 0 成功，-1 失败
 */
int step10b_venc_bind(VENC_CHN VeChn);

/**
 * 停止 VENC 编码通道
 * @param VeChn  VENC 通道编号
 */
void step10_venc_stop(VENC_CHN VeChn);

#endif /* STEP10_VENC_H */
