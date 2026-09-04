#include "step7_vpss.h"
#include "cvi_comm_vpss.h"

#include <cvi_vpss.h>
#include <stdio.h>
#include <string.h>

CVI_S32 step7_vpss_init(VPSS_GRP vpss_grp, CVI_U32 width, CVI_U32 height) {
    CVI_S32 ret;

    VPSS_GRP_ATTR_S vpss_grp_attr;
    memset(&vpss_grp_attr, 0, sizeof(vpss_grp_attr));
    vpss_grp_attr.u32MaxW = 1920;
    vpss_grp_attr.u32MaxH = 1080;
    vpss_grp_attr.enPixelFormat = PIXEL_FORMAT_NV21;
    vpss_grp_attr.u8VpssDev = 0;

    ret = CVI_VPSS_CreateGrp(vpss_grp, &vpss_grp_attr);
    if (ret != CVI_SUCCESS) {
        printf("[VPSS] CreateGrp failed: %#x\n", ret);
        return CVI_FAILURE;
    }

    ret = CVI_VPSS_ResetGrp(vpss_grp);
    if (ret != CVI_SUCCESS) {
        printf("[VPSS] ResetGrp failed: %#x\n", ret);
        return CVI_FAILURE;
    }

    /* CHN1: YUV420 1920x1080 -> VENC -> RTSP */
    VPSS_CHN_ATTR_S chn;
    memset(&chn, 0, sizeof(chn));
    chn.u32Width = width;
    chn.u32Height = height;
    chn.enVideoFormat = VIDEO_FORMAT_LINEAR;
    chn.enPixelFormat = PIXEL_FORMAT_YUV_PLANAR_420;
    chn.stFrameRate.s32SrcFrameRate = 30;
    chn.stFrameRate.s32DstFrameRate = 30;
    chn.u32Depth = 0;
    chn.stAspectRatio.enMode = ASPECT_RATIO_NONE;
    chn.stNormalize.bEnable = CVI_FALSE;
    chn.bFlip = CVI_TRUE;

    ret = CVI_VPSS_SetChnAttr(vpss_grp, VPSS_CHN1, &chn);
    if (ret != CVI_SUCCESS) {
        printf("[VPSS_CHN1] SetChnAttr: %#x\n", ret);
        return CVI_FAILURE;
    }
    ret = CVI_VPSS_EnableChn(vpss_grp, VPSS_CHN1);
    if (ret != CVI_SUCCESS) {
        printf("[VPSS_CHN1] EnableChn: %#x\n", ret);
        return CVI_FAILURE;
    }
    printf("[VPSS_CHN1] Init OK\n");
    return CVI_SUCCESS;
}

CVI_S32 step7b_vpss_start(VPSS_GRP vpss_grp) {
    CVI_S32 ret = CVI_VPSS_StartGrp(vpss_grp);
    if (ret != CVI_SUCCESS) {
        printf("[VPSS] StartGrp: %#x\n", ret);
        return CVI_FAILURE;
    }
    printf("[VPSS] Start OK\n");
    return CVI_SUCCESS;
}
