#ifndef STEP7_VPSS_H
#define STEP7_VPSS_H

#include <cvi_common.h>

/**
 * @brief Step7: VPSS init — create group, configure two output channels.
 *
 *   CHN0: NV21,  flip enabled  →  snapshot capture (Step9)
 *   CHN1: YUV420,flip enabled  →  VENC H.264 → RTSP streaming (Step10/11)
 *
 * Split into two functions:
 *   step7_vpss_init  — CreateGrp + SetChnAttr + EnableChn (before Bind)
 *   step7b_vpss_start — StartGrp (after Bind)
 */
CVI_S32 step7_vpss_init(VPSS_GRP vpss_grp, CVI_U32 width, CVI_U32 height);
CVI_S32 step7b_vpss_start(VPSS_GRP vpss_grp);

#endif
