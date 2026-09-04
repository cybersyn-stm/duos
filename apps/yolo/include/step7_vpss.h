#ifndef STEP7_VPSS_H
#define STEP7_VPSS_H

#include <cvi_common.h>

/* Step7: VPSS init
 *   CHN1: YUV420, flip enabled -> VENC H.264 -> RTSP streaming
 *   CHN2: added separately in yolo_gesture (640x640 NV21 -> TDL)
 *
 * step7_vpss_init  — CreateGrp + SetChnAttr + EnableChn
 * step7b_vpss_start — StartGrp
 */
CVI_S32 step7_vpss_init(VPSS_GRP vpss_grp, CVI_U32 width, CVI_U32 height);
CVI_S32 step7b_vpss_start(VPSS_GRP vpss_grp);

#endif
