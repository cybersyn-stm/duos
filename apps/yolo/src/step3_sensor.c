#include "step3_sensor.h"

#include <cvi_ae.h>
#include <cvi_awb.h>
#include <cvi_isp.h>
#include <cvi_sns_ctrl.h>
#include <stdio.h>
#include <string.h>
/*from:
 * /home/cybersyn/source_sdk/duo-buildroot-sdk-v2/device/generic/rootfs_overlay/duo256m/mnt/data/sensor_cfg_GC2083.ini*/
/*;section for source
[source]
;type = SOURCE_USER_FE
dev_num = 1
;section for sensor
[sensor]
;sensor name
name = GCORE_GC2083_MIPI_2M_30FPS_10BIT
;bus/i2c dev number
bus_id = 2
sns_i2c_addr = 37
mipi_dev = 0
lane_id = 1, 0, 2, -1, -1
pn_swap = 0, 0, 0, 0, 0*/
extern ISP_SNS_OBJ_S stSnsGc2083_Obj;
static ISP_SNS_OBJ_S *global_gc2083_obj = &stSnsGc2083_Obj;

static ISP_SNS_COMMBUS_U global_i2c_bus_info;
static ISP_SENSOR_EXP_FUNC_S global_sensor_exp_func;
static ISP_CMOS_SENSOR_IMAGE_MODE_S global_sensor_cmos_mode = {
    .u16Width = 1920,
    .u16Height = 1080,
    .f32Fps = 30.0f,
};

CVI_S32 step3_sensor_init(VI_PIPE vi_pipe) {
    CVI_S32 ret;

    ISP_INIT_ATTR_S init_attr;
    memset(&init_attr, 0, sizeof(init_attr));
    init_attr.enGainMode = SNS_GAIN_MODE_SHARE;
    init_attr.enSnsBdgMuxMode = SNS_BDG_MUX_NONE;
    if (global_gc2083_obj->pfnSetInit) {
        ret = global_gc2083_obj->pfnSetInit(vi_pipe, &init_attr);
        if (ret != CVI_SUCCESS) {
            printf("[SENSOR] SetInit fail: %#x\n", ret);
            return CVI_FAILURE;
        }
    }

    global_i2c_bus_info.s8I2cDev = 2;
    ret = global_gc2083_obj->pfnSetBusInfo(vi_pipe, global_i2c_bus_info);
    if (ret != CVI_SUCCESS) {
        printf("[SENSOR] BusInfo fail: %#x\n", ret);
        return CVI_FAILURE;
    }

    if (global_gc2083_obj->pfnPatchI2cAddr)
        global_gc2083_obj->pfnPatchI2cAddr(0x37);

    /* Register sensor I2C callbacks with AE/AWB libraries.
     * CRITICAL: Must be AFTER SetBusInfo so AE/AWB know the I2C bus.
     * Without this, AE cannot control sensor exposure → flickering + overexposed. */
    {
        ALG_LIB_S stAeLib = { .s32Id = vi_pipe };
        strncpy(stAeLib.acLibName, CVI_AE_LIB_NAME, sizeof(stAeLib.acLibName));
        ALG_LIB_S stAwbLib = { .s32Id = vi_pipe };
        strncpy(stAwbLib.acLibName, CVI_AWB_LIB_NAME, sizeof(stAwbLib.acLibName));

        if (global_gc2083_obj->pfnRegisterCallback) {
            ret = global_gc2083_obj->pfnRegisterCallback(vi_pipe, &stAeLib, &stAwbLib);
            if (ret != CVI_SUCCESS) {
                printf("[SENSOR] RegisterCallback fail: %#x\n", ret);
                return CVI_FAILURE;
            }
        } else {
            printf("[SENSOR] no RegisterCallback, AE/AWB won't work\n");
        }
    }

    if (global_gc2083_obj->pfnExpSensorCb)
        global_gc2083_obj->pfnExpSensorCb(&global_sensor_exp_func);

    if (global_sensor_exp_func.pfn_cmos_set_image_mode) {
        ret = global_sensor_exp_func.pfn_cmos_set_image_mode(
            vi_pipe, &global_sensor_cmos_mode);
        if (ret != CVI_SUCCESS) {
            printf("[SENSOR] ImageMode fail: %#x\n", ret);
            return CVI_FAILURE;
        }
    }

    /* Set WDR mode (sample does this, helps AE stability) */
    if (global_sensor_exp_func.pfn_cmos_set_wdr_mode) {
        ret = global_sensor_exp_func.pfn_cmos_set_wdr_mode(vi_pipe, WDR_MODE_NONE);
        if (ret != CVI_SUCCESS) {
            printf("[SENSOR] WDR fail: %#x\n", ret);
        }
    }

    /* NOTE: Probe moved to after MIPI clock enable — GC2083 I2C needs MIPI clock */
    printf("[SENSOR] OK\n");
    return CVI_SUCCESS;
}

CVI_S32 step3b_sensor_probe(VI_PIPE vi_pipe) {
    CVI_S32 ret;
    if (global_gc2083_obj->pfnSnsProbe) {
        ret = global_gc2083_obj->pfnSnsProbe(vi_pipe);
        if (ret != CVI_SUCCESS) {
            printf("[SENSOR] Probe fail: %#x\n", ret);
            return CVI_FAILURE;
        }
    }
    printf("[SENSOR] Probe OK\n");
    return CVI_SUCCESS;
}
