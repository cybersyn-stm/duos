#include "step6_isp.h"
#include "cvi_comm_3a.h"

#include <cvi_ae.h>
#include <cvi_awb.h>
#include <cvi_bin.h>
#include <cvi_isp.h>
#include <cvi_vi.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static pthread_t g_isp_tid;
static int g_isp_running = 0;

static void *isp_run_thread(void *arg) {
    VI_PIPE vi_pipe = (VI_PIPE)(uintptr_t)arg;
    printf("[ISP] Thread running...\n");
    CVI_ISP_Run(vi_pipe);
    printf("[ISP] Thread exit\n");
    g_isp_running = 0;
    return NULL;
}

void step6_isp_stop(VI_PIPE vi_pipe) {
    if (!g_isp_running)
        return;
    printf("[ISP] Stopping...\n");
    CVI_ISP_Exit(vi_pipe);
    pthread_join(g_isp_tid, NULL);
    printf("[ISP] Stopped\n");
}

CVI_S32 step6_isp_init(VI_PIPE vi_pipe) {
    CVI_S32 ret;

    ISP_BIND_ATTR_S isp_bind_attr;
    memset(&isp_bind_attr, 0, sizeof(ISP_BIND_ATTR_S));
    isp_bind_attr.stAeLib.s32Id = vi_pipe;
    snprintf(isp_bind_attr.stAeLib.acLibName,
             sizeof(isp_bind_attr.stAeLib.acLibName), "cvi_ae_lib");
    isp_bind_attr.stAwbLib.s32Id = vi_pipe;
    snprintf(isp_bind_attr.stAwbLib.acLibName,
             sizeof(isp_bind_attr.stAwbLib.acLibName), "cvi_awb_lib");
    isp_bind_attr.sensorId = 0;

    ISP_PUB_ATTR_S isp_pub_attr;
    memset(&isp_pub_attr, 0, sizeof(ISP_PUB_ATTR_S));
    isp_pub_attr.stSnsSize.u32Width = 1920;
    isp_pub_attr.stSnsSize.u32Height = 1080;
    isp_pub_attr.stWndRect.u32Width = 1920;
    isp_pub_attr.stWndRect.u32Height = 1080;
    isp_pub_attr.f32FrameRate = 30;
    isp_pub_attr.enBayer = BAYER_RGGB;
    isp_pub_attr.enWDRMode = WDR_MODE_NONE;

    {
        ALG_LIB_S stAeLib = {.s32Id = vi_pipe};
        strncpy(stAeLib.acLibName, CVI_AE_LIB_NAME, sizeof(stAeLib.acLibName));
        ret = CVI_AE_Register(vi_pipe, &stAeLib);
        if (ret != CVI_SUCCESS) {
            printf("[ISP] AE_Register failed: %#x\n", ret);
            return ret;
        }
    }
    {
        ALG_LIB_S stAwbLib = {.s32Id = vi_pipe};
        strncpy(stAwbLib.acLibName, CVI_AWB_LIB_NAME,
                sizeof(stAwbLib.acLibName));
        ret = CVI_AWB_Register(vi_pipe, &stAwbLib);
        if (ret != CVI_SUCCESS) {
            printf("[ISP] AWB_Register failed: %#x\n", ret);
            return ret;
        }
    }

    ret = CVI_ISP_SetBindAttr(vi_pipe, &isp_bind_attr);
    if (ret != CVI_SUCCESS) {
        printf("[ISP] SetBindAttr failed: %#x\n", ret);
        return ret;
    }

    ret = CVI_ISP_MemInit(vi_pipe);
    if (ret != CVI_SUCCESS) {
        printf("[ISP] MemInit failed\n");
        return ret;
    }

    ret = CVI_ISP_SetPubAttr(vi_pipe, &isp_pub_attr);
    if (ret != CVI_SUCCESS) {
        printf("[ISP] SetPubAttr failed\n");
        return ret;
    }

    ret = CVI_ISP_Init(vi_pipe);
    if (ret != CVI_SUCCESS) {
        printf("[ISP] Init failed\n");
        return ret;
    }
    printf("[ISP] Init OK\n");

    /* Load ISP tuning parameters (AE/AWB/CCM/gamma/noise etc.).
     * Standard sample_vi_fd does this via SAMPLE_COMM_BIN_ReadParaFrombin().
     * Without this, AE/AWB use generic defaults → poor exposure & color. */
    // 获取ISP优化
    {
        /*
        CVI_CHAR bin_name[BIN_FILE_LENGTH] = {0};
        ret = CVI_BIN_GetBinName(bin_name);
        if (ret == CVI_SUCCESS) {
            printf("[ISP] Loading tuning params from '%s'\n", bin_name);
            FILE *fp = fopen(bin_name, "rb");
            if (fp) {
                fseek(fp, 0, SEEK_END);
                long sz = ftell(fp);
                rewind(fp);
                CVI_U8 *buf = (CVI_U8 *)malloc(sz);
                if (buf) {
                    if (fread(buf, 1, sz, fp) == (size_t)sz) {
                        CVI_S32 bin_ret =
                            CVI_BIN_ImportBinData(buf, (CVI_U32)sz);
                        if (bin_ret == CVI_SUCCESS)
                            printf("[ISP] Tuning params loaded (%ld bytes)\n",
                                   sz);
                        else
                            printf("[ISP] Tuning import failed: %#x (using "
                                   "defaults)\n",
                                   bin_ret);
                    }
                    free(buf);
                }
                fclose(fp);
            } else {
                printf("[ISP] No tuning file '%s' (using defaults)\n",
                       bin_name);
            }
        } else {
            printf("[ISP] Cannot get bin path (using defaults)\n");
        }
        */
        // 不生成内存
        CVI_CHAR bin_naame[BIN_FILE_LENGTH] = {0};
        ret = CVI_BIN_GetBinName(bin_naame);
        if (ret == CVI_SUCCESS) {
            printf("[ISP] Loading tuning params from '%s'\n", bin_naame);
            FILE *fp = fopen(bin_naame, "rb");
            if (fp) {
                if (fseek(fp, 0, SEEK_END) == 0) {
                    int file_size = ftell(fp);
                    rewind(fp);
                    if (file_size > 0) {
                        CVI_U8 *buf = (CVI_U8 *)malloc(file_size);
                        fread(buf, 1, file_size, fp);
                        if (sizeof(buf) == 0) {
                            printf("[ISP] create buf failed\n");
                        }
                        ret = CVI_BIN_ImportBinData(buf, (CVI_U32)file_size);
                        if (ret == CVI_SUCCESS) {
                            printf("[ISP] Tuning params loaded (%d bytes)\n",
                                   file_size);
                        } else {
                            printf("[ISP] Tuning import failed: %#x (using "
                                   "defaults)\n",
                                   ret);
                        }
                        free(buf);
                    }
                }
            }
            fclose(fp);
        }
    }

    /* VI channel setup (BEFORE ISP_Run — ISP needs the channel ready) */
    {
        VI_CHN_ATTR_S chn_attr;
        memset(&chn_attr, 0, sizeof(chn_attr));
        chn_attr.stSize.u32Width = 1920;
        chn_attr.stSize.u32Height = 1080;
        chn_attr.enPixelFormat = PIXEL_FORMAT_NV21;
        chn_attr.enDynamicRange = DYNAMIC_RANGE_SDR8;
        chn_attr.enVideoFormat = VIDEO_FORMAT_LINEAR;
        chn_attr.enCompressMode = COMPRESS_MODE_NONE;

        ret = CVI_VI_SetChnAttr(vi_pipe, 0, &chn_attr);
        if (ret != CVI_SUCCESS) {
            printf("[ISP] SetChnAttr failed\n");
            return ret;
        }

        ret = CVI_VI_EnableChn(vi_pipe, 0);
        if (ret != CVI_SUCCESS) {
            printf("[ISP] EnableChn failed\n");
            return ret;
        }
    }

    /* CVI_ISP_Run is blocking — run in a separate thread
     * (VI channel must be set up first, otherwise ISP_Run may segfault) */
    {
        g_isp_running = 1;
        if (pthread_create(&g_isp_tid, NULL, isp_run_thread,
                           (void *)(uintptr_t)vi_pipe) != 0) {
            printf("[ISP] thread create failed\n");
            return CVI_FAILURE;
        }
    }

    return CVI_SUCCESS;
}
