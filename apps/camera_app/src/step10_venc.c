/**
 * Step 10: VENC H.264 编码器实现
 *
 * H.264 Baseline, CBR 1800kbps, GOP60, 1080p@30fps
 */

#include <stdio.h>
#include <string.h>
#include "step10_venc.h"
#include "step7_vpss.h"
#include "linux/cvi_comm_video.h"
#include "cvi_comm_vb.h"
#include "linux/cvi_comm_venc.h"
#include "linux/cvi_comm_rc.h"
#include "cvi_venc.h"
#include "cvi_sys.h"

int step10_venc_init(VENC_CHN VeChn)
{
    VENC_CHN_ATTR_S     stChnAttr;
    VENC_RECV_PIC_PARAM_S stRecvParam;
    CVI_S32 s32Ret;

    printf("[VENC] 初始化 H.264 编码器 (CBR 1800kbps, GOP60)...\n");

    /* ---------- VENC Channel 属性 ---------- */
    memset(&stChnAttr, 0, sizeof(stChnAttr));

    /* VENC_ATTR_S */
    stChnAttr.stVencAttr.enType          = PT_H264;
    stChnAttr.stVencAttr.u32MaxPicWidth  = VPSS_STREAM_W;
    stChnAttr.stVencAttr.u32MaxPicHeight = VPSS_STREAM_H;
    stChnAttr.stVencAttr.u32PicWidth     = VPSS_STREAM_W;
    stChnAttr.stVencAttr.u32PicHeight    = VPSS_STREAM_H;
    stChnAttr.stVencAttr.u32BufSize      = VPSS_STREAM_W * VPSS_STREAM_H;
    stChnAttr.stVencAttr.u32Profile      = 0;  /* Baseline */
    stChnAttr.stVencAttr.bByFrame        = CVI_TRUE;
    stChnAttr.stVencAttr.bSingleCore     = CVI_FALSE;
    stChnAttr.stVencAttr.bEsBufQueueEn   = CVI_TRUE;
    stChnAttr.stVencAttr.bIsoSendFrmEn   = CVI_FALSE;

    /* H.264 特有属性 */
    stChnAttr.stVencAttr.stAttrH264e.bRcnRefShareBuf = CVI_FALSE;
    stChnAttr.stVencAttr.stAttrH264e.bSingleLumaBuf  = CVI_FALSE;

    /* RC 属性: H264 CBR */
    stChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
    stChnAttr.stRcAttr.stH264Cbr.u32Gop            = 60;
    stChnAttr.stRcAttr.stH264Cbr.u32StatTime       = 1;
    stChnAttr.stRcAttr.stH264Cbr.u32SrcFrameRate   = 30;
    stChnAttr.stRcAttr.stH264Cbr.fr32DstFrameRate  = 30;
    stChnAttr.stRcAttr.stH264Cbr.u32BitRate        = 1800;
    stChnAttr.stRcAttr.stH264Cbr.bVariFpsEn        = CVI_FALSE;

    /* GOP 属性 */
    stChnAttr.stGopAttr.enGopMode = VENC_GOPMODE_NORMALP;
    stChnAttr.stGopAttr.stNormalP.s32IPQpDelta = 2;

    s32Ret = CVI_VENC_CreateChn(VeChn, &stChnAttr);
    if (s32Ret != CVI_SUCCESS) {
        fprintf(stderr, "[ERROR] CVI_VENC_CreateChn: 0x%x\n", s32Ret);
        return -1;
    }

    /* ---------- 开始接收帧 (连续模式) ---------- */
    stRecvParam.s32RecvPicNum = -1;  /* 持续接收 */
    s32Ret = CVI_VENC_StartRecvFrame(VeChn, &stRecvParam);
    if (s32Ret != CVI_SUCCESS) {
        fprintf(stderr, "[ERROR] CVI_VENC_StartRecvFrame: 0x%x\n", s32Ret);
        CVI_VENC_DestroyChn(VeChn);
        return -1;
    }

    printf("[VENC] H.264 编码器初始化完成\n");
    return 0;
}

int step10b_venc_bind(VENC_CHN VeChn)
{
    MMF_CHN_S stSrcChn, stDestChn;

    stSrcChn.enModId  = CVI_ID_VPSS;
    stSrcChn.s32DevId = VPSS_GRP_NUM;
    stSrcChn.s32ChnId = VPSS_CHN_STREAM;  /* CHN1 → VENC */

    stDestChn.enModId  = CVI_ID_VENC;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = VeChn;

    printf("[BIND] VPSS(Grp%d CHN%d) → VENC(CHN%d)\n",
           VPSS_GRP_NUM, VPSS_CHN_STREAM, VeChn);

    return CVI_SYS_Bind(&stSrcChn, &stDestChn);
}

void step10_venc_stop(VENC_CHN VeChn)
{
    /* 解绑 */
    MMF_CHN_S stSrcChn, stDestChn;
    stSrcChn.enModId  = CVI_ID_VPSS;
    stSrcChn.s32DevId = VPSS_GRP_NUM;
    stSrcChn.s32ChnId = VPSS_CHN_STREAM;
    stDestChn.enModId  = CVI_ID_VENC;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = VeChn;
    CVI_SYS_UnBind(&stSrcChn, &stDestChn);

    CVI_VENC_StopRecvFrame(VeChn);
    CVI_VENC_DestroyChn(VeChn);
    printf("[VENC] 已停止\n");
}
