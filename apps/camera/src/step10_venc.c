#include "step10_venc.h"

#include <stdio.h>
#include <string.h>

#include <cvi_comm_vb.h>
#include <cvi_comm_venc.h>
#include <cvi_comm_rc.h>
#include <cvi_sys.h>
#include <cvi_venc.h>

CVI_S32 step10_venc_init(VENC_CHN venc_chn, CVI_U32 width, CVI_U32 height) {
    CVI_S32 ret;

    VENC_CHN_ATTR_S venc_attr;
    memset(&venc_attr, 0, sizeof(venc_attr));
    venc_attr.stVencAttr.enType = PT_H264;
    venc_attr.stVencAttr.u32MaxPicWidth = width;
    venc_attr.stVencAttr.u32MaxPicHeight = height;
    venc_attr.stVencAttr.u32PicWidth = width;
    venc_attr.stVencAttr.u32PicHeight = height;
    venc_attr.stVencAttr.u32Profile = 0;
    venc_attr.stVencAttr.bByFrame = CVI_TRUE;
    venc_attr.stVencAttr.bSingleCore = CVI_FALSE;

    venc_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
    venc_attr.stRcAttr.stH264Cbr.u32Gop = 60;
    venc_attr.stRcAttr.stH264Cbr.u32BitRate = 1800;
    venc_attr.stRcAttr.stH264Cbr.fr32DstFrameRate = 30;
    venc_attr.stRcAttr.stH264Cbr.u32SrcFrameRate = 30;
    venc_attr.stRcAttr.stH264Cbr.u32StatTime = 1;

    venc_attr.stGopAttr.enGopMode = VENC_GOPMODE_NORMALP;

    ret = CVI_VENC_CreateChn(venc_chn, &venc_attr);
    if (ret != CVI_SUCCESS) {
        printf("[VENC] CreateChn failed: %#x\n", ret);
        return CVI_FAILURE;
    }

    VENC_RECV_PIC_PARAM_S recv_param;
    recv_param.s32RecvPicNum = -1;  /* 手动 SendFrame 模式，使用 SDK 默认值 */

    ret = CVI_VENC_StartRecvFrame(venc_chn, &recv_param);
    if (ret != CVI_SUCCESS) {
        printf("[VENC] StartRecvFrame failed: %#x\n", ret);
        return CVI_FAILURE;
    }

    printf("[VENC] Init OK (H264 %ux%u CBR 1800kbps GOP60)\n", width, height);
    return CVI_SUCCESS;
}

CVI_S32 step10b_venc_bind(VPSS_GRP vpss_grp, VPSS_CHN vpss_chn, VENC_CHN venc_chn) {
    CVI_S32 ret;

    MMF_CHN_S src_chn;
    memset(&src_chn, 0, sizeof(src_chn));
    src_chn.enModId = CVI_ID_VPSS;
    src_chn.s32DevId = vpss_grp;
    src_chn.s32ChnId = vpss_chn;

    MMF_CHN_S dest_chn;
    memset(&dest_chn, 0, sizeof(dest_chn));
    dest_chn.enModId = CVI_ID_VENC;
    dest_chn.s32DevId = 0;
    dest_chn.s32ChnId = venc_chn;

    ret = CVI_SYS_Bind(&src_chn, &dest_chn);
    if (ret != CVI_SUCCESS) {
        printf("[VENC-BIND] VPSS_CHN%d -> VENC_CHN%d failed: %#x\n", vpss_chn, venc_chn, ret);
        return CVI_FAILURE;
    }
    printf("[VENC-BIND] VPSS_CHN%d -> VENC_CHN%d OK\n", vpss_chn, venc_chn);
    return CVI_SUCCESS;
}

CVI_S32 step10_venc_stop(VENC_CHN venc_chn) {
    CVI_S32 ret;

    ret = CVI_VENC_StopRecvFrame(venc_chn);
    if (ret != CVI_SUCCESS)
        printf("[VENC] StopRecvFrame warn: %#x\n", ret);

    ret = CVI_VENC_DestroyChn(venc_chn);
    if (ret != CVI_SUCCESS) {
        printf("[VENC] DestroyChn failed: %#x\n", ret);
        return CVI_FAILURE;
    }
    printf("[VENC] Stop OK\n");
    return CVI_SUCCESS;
}
