#include "step2_mipi.h"

#include <cvi_isp.h>
#include <cvi_sns_ctrl.h>
#include <cvi_sys.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

extern ISP_SNS_OBJ_S stSnsGc2083_Obj;
static ISP_SNS_OBJ_S *global_gc2083_obj = &stSnsGc2083_Obj;

CVI_S32 step2_mipi_init(VI_PIPE vi_pipe) {
    SNS_COMBO_DEV_ATTR_S mipi_rx_attr;
    CVI_S32 ret;
    CVI_U32 devno = 0;

    memset(&mipi_rx_attr, 0, sizeof(mipi_rx_attr));
    // 获取传感器的属性
    ret = global_gc2083_obj->pfnGetRxAttr(vi_pipe, &mipi_rx_attr);
    if (ret != CVI_SUCCESS) {
        printf("[MIPI] GetRxAttr failed: %#x\n", ret);
        return CVI_FAILURE;
    }

    devno = mipi_rx_attr.devno;

    if (CVI_MIPI_SetSensorReset(devno, 1)) {
        printf("[MIPI] Reset fail\n");
        return CVI_FAILURE;
    }
    if (CVI_MIPI_SetMipiReset(devno, 1)) {
        printf("[MIPI] MipiReset fail\n");
        return CVI_FAILURE;
    }
    if (CVI_MIPI_SetMipiAttr(vi_pipe, (CVI_VOID *)&mipi_rx_attr)) {
        printf("[MIPI] Attr fail\n");
        return CVI_FAILURE;
    }
    if (CVI_MIPI_SetSensorClock(devno, 1)) {
        printf("[MIPI] Clock fail\n");
        return CVI_FAILURE;
    }

    usleep(50);

    if (CVI_MIPI_SetSensorReset(devno, 0)) {
        printf("[MIPI] Unreset fail\n");
        return CVI_FAILURE;
    }
    printf("[MIPI] OK\n");

    return CVI_SUCCESS;
}
