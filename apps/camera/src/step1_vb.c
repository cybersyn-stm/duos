#include "step1_vb.h"

#include <cvi_comm_vb.h>
#include <cvi_sys.h>
#include <cvi_vb.h>
#include <stdio.h>
#include <string.h>

#define SENSOR_WIDTH 1920
#define SENSOR_HEIGHT 1080

CVI_S32 step1_vb_init(void) {
    VB_CONFIG_S vb_config;
    CVI_S32 ret;

    memset(&vb_config, 0, sizeof(VB_CONFIG_S));
    vb_config.u32MaxPoolCnt = 1;
    vb_config.astCommPool[0].u32BlkSize = SENSOR_WIDTH * SENSOR_HEIGHT * 3 / 2;
    /* YUV420/NV21 尺寸相同: 1920×1080×3/2 = 3,110,400 */
    vb_config.astCommPool[0].u32BlkCnt = 12;  /* CHN0(depth=4)+CHN1(depth=3)+ISP+margin=12
                                                * 剩余 carveout 给 VENC 重建缓冲区 */

    ret = CVI_VB_SetConfig(&vb_config);
    if (ret != CVI_SUCCESS) {
        printf("[VB] Config fail: %#x\n", ret);
        return CVI_FAILURE;
    }

    ret = CVI_VB_GetConfig(&vb_config);
    if (ret != CVI_SUCCESS) {
        printf("[VB] GetConfig fail: %#x\n", ret);
        return CVI_FAILURE;
    }

    ret = CVI_VB_Init();
    if (ret != CVI_SUCCESS) {
        printf("[VB] Init fail\n");
        return CVI_FAILURE;
    }
    printf("[VB] OK\n");

    return CVI_SUCCESS;
}
