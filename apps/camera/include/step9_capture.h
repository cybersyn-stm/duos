#ifndef STEP9_CAPTURE_H
#define STEP9_CAPTURE_H

#include <cvi_common.h>
#include <cvi_vpss.h>

/* 单帧捕获（保留兼容） */
CVI_S32 step9_capture_init(VPSS_GRP vpss_grp, VPSS_CHN vpss_chn);

/* 批量连拍：base_001.yuv … base_NNN.yuv，跳过零帧，全速
 * run 指向 g_run 标志，为 0 时中止 */
CVI_S32 step9_capture_sequence(VPSS_GRP vpss_grp, VPSS_CHN vpss_chn,
                                const char *base, int count,
                                volatile int *run);

#endif
