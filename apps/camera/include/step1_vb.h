#ifndef STEP1_VB_H
#define STEP1_VB_H

#include <cvi_common.h>

/**
 * @brief Step1: Initialize video buffer (VB) pool.
 *
 * Allocates a pool of contiguous physical memory buffers shared by
 * VI/ISP/VPSS in the pipeline. One buffer = one NV21 frame.
 *
 * Parameters:
 *   BlkSize = 1920 × 1080 × 3/2 = 3,110,400 bytes (one NV21 frame)
 *   BlkCnt  = 5  (pool depth, enough for single-frame capture)
 *
 * @return CVI_SUCCESS on success, negative on error.
 */

// 初始化VB(Video Buffer)大小
CVI_S32 step1_vb_init(void);

#endif /* STEP1_VB_H */
