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

    VPSS_CHN_ATTR_S vpss_chn_attr[VPSS_MAX_PHY_CHN_NUM];
    memset(vpss_chn_attr, 0, sizeof(vpss_chn_attr));
    vpss_chn_attr[VPSS_CHN0].u32Width = width;
    vpss_chn_attr[VPSS_CHN0].u32Height = height;
    vpss_chn_attr[VPSS_CHN0].enVideoFormat = VIDEO_FORMAT_LINEAR;
    vpss_chn_attr[VPSS_CHN0].enPixelFormat = PIXEL_FORMAT_NV21;
    vpss_chn_attr[VPSS_CHN0].stFrameRate.s32SrcFrameRate = 30;
    vpss_chn_attr[VPSS_CHN0].stFrameRate.s32DstFrameRate = 30;
    vpss_chn_attr[VPSS_CHN0].u32Depth = 4;
    vpss_chn_attr[VPSS_CHN0].stAspectRatio.enMode = ASPECT_RATIO_NONE;
    vpss_chn_attr[VPSS_CHN0].stNormalize.bEnable = CVI_FALSE;
    vpss_chn_attr[VPSS_CHN0].bFlip = CVI_TRUE;

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

    ret = CVI_VPSS_SetChnAttr(vpss_grp, VPSS_CHN0, &vpss_chn_attr[VPSS_CHN0]);
    if (ret != CVI_SUCCESS) {
        printf("[VPSS] SetChnAttr failed: %#x\n", ret);
        return CVI_FAILURE;
    }

    ret = CVI_VPSS_EnableChn(vpss_grp, VPSS_CHN0);
    if (ret != CVI_SUCCESS) {
        printf("[VPSS_CHN0] EnableChn failed: %#x\n", ret);
        return CVI_FAILURE;
    }
    printf("[VPSS_CHN0] Init OK\n");

    /* CHN1: YUV420 for VENC streaming */
    vpss_chn_attr[VPSS_CHN1].u32Width = width;
    vpss_chn_attr[VPSS_CHN1].u32Height = height;
    vpss_chn_attr[VPSS_CHN1].enVideoFormat = VIDEO_FORMAT_LINEAR;
    vpss_chn_attr[VPSS_CHN1].enPixelFormat = PIXEL_FORMAT_YUV_PLANAR_420;
    vpss_chn_attr[VPSS_CHN1].stFrameRate.s32SrcFrameRate = 30;
    vpss_chn_attr[VPSS_CHN1].stFrameRate.s32DstFrameRate = 30;
    vpss_chn_attr[VPSS_CHN1].u32Depth = 3;
    vpss_chn_attr[VPSS_CHN1].stAspectRatio.enMode = ASPECT_RATIO_NONE;
    vpss_chn_attr[VPSS_CHN1].stNormalize.bEnable = CVI_FALSE;
    vpss_chn_attr[VPSS_CHN1].bFlip = CVI_TRUE;

    ret = CVI_VPSS_SetChnAttr(vpss_grp, VPSS_CHN1, &vpss_chn_attr[VPSS_CHN1]);
    if (ret != CVI_SUCCESS) {
        printf("[VPSS_CHN1] SetChnAttr failed: %#x\n", ret);
        return CVI_FAILURE;
    }
    ret = CVI_VPSS_EnableChn(vpss_grp, VPSS_CHN1);
    if (ret != CVI_SUCCESS) {
        printf("[VPSS_CHN1] EnableChn failed: %#x\n", ret);
        return CVI_FAILURE;
    }
    printf("[VPSS_CHN1] Init OK\n");

    return CVI_SUCCESS;
}

/* StartGrp must be called AFTER bind (SDK: Bind → StartGrp, not StartGrp →
 * Bind) */
CVI_S32 step7b_vpss_start(VPSS_GRP vpss_grp) {
    CVI_S32 ret;
    ret = CVI_VPSS_StartGrp(vpss_grp);
    if (ret != CVI_SUCCESS) {
        printf("[VPSS] StartGrp failed: %#x\n", ret);
        return CVI_FAILURE;
    }
    printf("[VPSS] Start OK\n");
    return CVI_SUCCESS;
}
