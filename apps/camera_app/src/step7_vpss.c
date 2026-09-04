/**
 * Step 7: VPSS 双通道实现
 *
 * CHN0 (SNAP):   640x480  NV12              — 拍照快照
 * CHN1 (STREAM): 1920x1080 YUV_PLANAR_420   — VENC H.264 编码
 */

#include <stdio.h>
#include <string.h>
#include "step7_vpss.h"
#include "linux/cvi_comm_video.h"
#include "cvi_vpss.h"
#include "cvi_sys.h"

int step7_vpss_init(VPSS_GRP VpssGrp)
{
    VPSS_GRP_ATTR_S stGrpAttr;
    VPSS_CHN_ATTR_S stChnAttr;
    CVI_S32 s32Ret;

    printf("[VPSS] 初始化双通道 (CHN0: %dx%d NV12, CHN1: %dx%d YUV420)...\n",
           VPSS_SNAP_W, VPSS_SNAP_H, VPSS_STREAM_W, VPSS_STREAM_H);

    /* ---------- Group 属性 ---------- */
    memset(&stGrpAttr, 0, sizeof(stGrpAttr));
    stGrpAttr.u32MaxW       = VPSS_IN_W;
    stGrpAttr.u32MaxH       = VPSS_IN_H;
    stGrpAttr.enPixelFormat = PIXEL_FORMAT_YUV_PLANAR_420;
    stGrpAttr.stFrameRate.s32SrcFrameRate = -1;
    stGrpAttr.stFrameRate.s32DstFrameRate = -1;
    stGrpAttr.u8VpssDev     = 0;

    s32Ret = CVI_VPSS_CreateGrp(VpssGrp, &stGrpAttr);
    if (s32Ret != CVI_SUCCESS) {
        fprintf(stderr, "[ERROR] CVI_VPSS_CreateGrp: 0x%x\n", s32Ret);
        return -1;
    }

    /* ---------- CHN0: 快照通道 (NV12) ---------- */
    memset(&stChnAttr, 0, sizeof(stChnAttr));
    stChnAttr.u32Width       = VPSS_SNAP_W;
    stChnAttr.u32Height      = VPSS_SNAP_H;
    stChnAttr.enVideoFormat  = VIDEO_FORMAT_LINEAR;
    stChnAttr.enPixelFormat  = PIXEL_FORMAT_NV12;
    stChnAttr.u32Depth       = 1;
    stChnAttr.stFrameRate.s32SrcFrameRate = -1;
    stChnAttr.stFrameRate.s32DstFrameRate = -1;
    stChnAttr.bFlip          = CVI_TRUE;

    s32Ret = CVI_VPSS_SetChnAttr(VpssGrp, VPSS_CHN_SNAP, &stChnAttr);
    if (s32Ret != CVI_SUCCESS) {
        fprintf(stderr, "[ERROR] VPSS CHN0 SetChnAttr: 0x%x\n", s32Ret);
        return -1;
    }

    s32Ret = CVI_VPSS_EnableChn(VpssGrp, VPSS_CHN_SNAP);
    if (s32Ret != CVI_SUCCESS) {
        fprintf(stderr, "[ERROR] VPSS CHN0 EnableChn: 0x%x\n", s32Ret);
        return -1;
    }

    /* ---------- CHN1: 编码通道 (YUV_PLANAR_420, VENC 需要) ---------- */
    memset(&stChnAttr, 0, sizeof(stChnAttr));
    stChnAttr.u32Width       = VPSS_STREAM_W;
    stChnAttr.u32Height      = VPSS_STREAM_H;
    stChnAttr.enVideoFormat  = VIDEO_FORMAT_LINEAR;
    stChnAttr.enPixelFormat  = PIXEL_FORMAT_YUV_PLANAR_420;
    stChnAttr.u32Depth       = 1;
    stChnAttr.stFrameRate.s32SrcFrameRate = -1;
    stChnAttr.stFrameRate.s32DstFrameRate = -1;
    stChnAttr.bFlip          = CVI_TRUE;

    s32Ret = CVI_VPSS_SetChnAttr(VpssGrp, VPSS_CHN_STREAM, &stChnAttr);
    if (s32Ret != CVI_SUCCESS) {
        fprintf(stderr, "[ERROR] VPSS CHN1 SetChnAttr: 0x%x\n", s32Ret);
        return -1;
    }

    s32Ret = CVI_VPSS_EnableChn(VpssGrp, VPSS_CHN_STREAM);
    if (s32Ret != CVI_SUCCESS) {
        fprintf(stderr, "[ERROR] VPSS CHN1 EnableChn: 0x%x\n", s32Ret);
        return -1;
    }

    printf("[VPSS] 双通道初始化完成\n");
    return 0;
}

int step7b_vpss_start(VPSS_GRP VpssGrp)
{
    CVI_S32 s32Ret = CVI_VPSS_StartGrp(VpssGrp);
    if (s32Ret != CVI_SUCCESS) {
        fprintf(stderr, "[ERROR] CVI_VPSS_StartGrp: 0x%x\n", s32Ret);
        return -1;
    }
    printf("[VPSS] Group 启动完成\n");
    return 0;
}

int step7c_bind_vi_to_vpss(void)
{
    MMF_CHN_S stSrcChn, stDestChn;

    stSrcChn.enModId  = CVI_ID_VI;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = 0;

    stDestChn.enModId  = CVI_ID_VPSS;
    stDestChn.s32DevId = VPSS_GRP_NUM;
    stDestChn.s32ChnId = 0;

    printf("[BIND] VI(Pipe0) → VPSS(Grp%d)\n", VPSS_GRP_NUM);

    return CVI_SYS_Bind(&stSrcChn, &stDestChn);
}

void step7_vpss_cleanup(VPSS_GRP VpssGrp)
{
    CVI_VPSS_StopGrp(VpssGrp);
    CVI_VPSS_DisableChn(VpssGrp, VPSS_CHN_SNAP);
    CVI_VPSS_DisableChn(VpssGrp, VPSS_CHN_STREAM);
    CVI_VPSS_DestroyGrp(VpssGrp);
    printf("[VPSS] 已清理\n");
}
