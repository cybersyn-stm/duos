#ifndef STEP10_VENC_H
#define STEP10_VENC_H

#include <cvi_common.h>

CVI_S32 step10_venc_init(VENC_CHN venc_chn, CVI_U32 width, CVI_U32 height);
CVI_S32 step10b_venc_bind(VPSS_GRP vpss_grp, VPSS_CHN vpss_chn, VENC_CHN venc_chn);
CVI_S32 step10_venc_stop(VENC_CHN venc_chn);

#endif
