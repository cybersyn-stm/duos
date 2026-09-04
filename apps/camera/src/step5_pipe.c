#include "step5_pipe.h"
#include "cvi_type.h"

#include <cvi_vi.h>
#include <stdio.h>
#include <string.h>

CVI_S32 step5_pipe_init(VI_PIPE vi_pipe) {
    CVI_S32 ret;
    VI_PIPE_ATTR_S pipe_attr;
    memset(&pipe_attr, 0, sizeof(pipe_attr));
    pipe_attr.u32MaxW = 1920;
    pipe_attr.u32MaxH = 1080;
    pipe_attr.enPixFmt = PIXEL_FORMAT_RGB_BAYER_12BPP;
    pipe_attr.enBitWidth = DATA_BITWIDTH_12;
    pipe_attr.bNrEn = CVI_TRUE;
    pipe_attr.bYuvBypassPath = CVI_FALSE;
    pipe_attr.enCompressMode = COMPRESS_MODE_NONE;
    pipe_attr.stFrameRate.s32SrcFrameRate = -1;
    pipe_attr.stFrameRate.s32DstFrameRate = -1;

    ret = CVI_VI_CreatePipe(vi_pipe, &pipe_attr);
    if (ret != CVI_SUCCESS) { printf("[VI_PIPE] Create failed: %#x\n", ret); return CVI_FAILURE; }

    ret = CVI_VI_StartPipe(vi_pipe);
    if (ret != CVI_SUCCESS) { printf("[VI_PIPE] Start failed: %#x\n", ret); return CVI_FAILURE; }
    printf("[VI_PIPE] OK\n");

    return CVI_SUCCESS;
}
