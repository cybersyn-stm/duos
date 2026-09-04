#include "step4_vi.h"
#include <cvi_sys.h>
#include <cvi_vi.h>

#include <stdio.h>
#include <string.h>
CVI_S32 step4_VI_init(VI_DEV vi_dev) {
    CVI_S32 ret;

    VI_DEV_ATTR_S vi_dev_attr;
    memset(&vi_dev_attr, 0, sizeof(vi_dev_attr));
    vi_dev_attr.enIntfMode = VI_MODE_MIPI; // RAW
    vi_dev_attr.enWorkMode = VI_WORK_MODE_1Multiplex;
    vi_dev_attr.stSize.u32Width = 1920;
    vi_dev_attr.stSize.u32Height = 1080;
    vi_dev_attr.enBayerFormat = BAYER_FORMAT_RG;
    vi_dev_attr.enInputDataType = VI_DATA_TYPE_RGB;
    vi_dev_attr.stWDRAttr.enWDRMode = WDR_MODE_NONE;
    vi_dev_attr.stWDRAttr.u32CacheLine = 1080;
    vi_dev_attr.snrFps = 30;

    VI_DEV_BIND_PIPE_S dev_bind_pipe;
    memset(&dev_bind_pipe, 0, sizeof(dev_bind_pipe));
    dev_bind_pipe.u32Num = 1;
    dev_bind_pipe.PipeId[0] = vi_dev;

    ret = CVI_VI_SetDevAttr(vi_dev, &vi_dev_attr);
    if (ret != CVI_SUCCESS) { printf("[VI] SetDevAttr fail: %#x\n", ret); return CVI_FAILURE; }

    ret = CVI_VI_EnableDev(vi_dev);
    if (ret != CVI_SUCCESS) { printf("[VI] EnableDev fail: %#x\n", ret); return CVI_FAILURE; }

    ret = CVI_VI_SetDevBindPipe(vi_dev, &dev_bind_pipe);
    if (ret != CVI_SUCCESS) { printf("[VI] BindPipe fail: %#x\n", ret); return CVI_FAILURE; }
    printf("[VI] OK\n");
    return CVI_SUCCESS;
}
