#define _GNU_SOURCE
#include "cvi_draw_rect.h"
#include "cvi_tdl.h"
#include "step0_reset.h"
#include "step10_venc.h"
#include "step11_rtsp.h"
#include "step1_vb.h"
#include "step2_mipi.h"
#include "step3_sensor.h"
#include "step4_vi.h"
#include "step5_pipe.h"
#include "step6_isp.h"
#include "step7_vpss.h"
#include "step8_bind.h"
#include <cvi_comm_venc.h>
#include <cvi_comm_vpss.h>
#include <cvi_sys.h>
#include <cvi_venc.h>
#include <cvi_vpss.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
/* ---- Globals ---- */
static volatile int g_run;
static CVI_RTSP_CTX *g_rtsp;
static CVI_RTSP_SESSION *g_ses;
static const char *g_names[] = {"one", "two", "three", "four", "five"};
/* ---- Shared detection results (mutex-protected) ---- */
static pthread_mutex_t g_det_mutex = PTHREAD_MUTEX_INITIALIZER;
static int g_det_count = 0;
#define MAX_DETS 20
static struct {
    float x1, y1, x2, y2;
    float score;
    int cls;
} g_dets[MAX_DETS];
static void on_sig(int s) {
    if (s == SIGINT || s == SIGTERM)
        g_run = 0;
}
/* ---- RTSP streaming (with detection overlay) ---- */
static void *stream_thread(void *arg) {

    (void)arg;
    while (g_run) {
        VIDEO_FRAME_INFO_S f;
        if (CVI_VPSS_GetChnFrame(0, VPSS_CHN1, &f, 1000) != CVI_SUCCESS) {
            usleep(10000);
            continue;
        }
        /* Draw detection boxes on frame before encoding */
        pthread_mutex_lock(&g_det_mutex);
        if (g_det_count > 0) {

            cvtdl_object_t obj = {0};

            obj.size = g_det_count;

            obj.width = 1920;

            obj.height = 1080;

            obj.rescale_type = RESCALE_NOASPECT;

            obj.info = (cvtdl_object_info_t *)malloc(
                sizeof(cvtdl_object_info_t) * obj.size);

            if (obj.info) {

                memset(obj.info, 0, sizeof(cvtdl_object_info_t) * obj.size);

                for (int i = 0; i < g_det_count; i++) {

                    snprintf(obj.info[i].name, 128, "%s %.0f%%",

                             g_names[g_dets[i].cls], g_dets[i].score * 100);

                    obj.info[i].bbox.x1 = g_dets[i].x1 * 3.0f;
                    obj.info[i].bbox.y1 = g_dets[i].y1 * 1.6875f;
                    obj.info[i].bbox.x2 = g_dets[i].x2 * 3.0f;
                    obj.info[i].bbox.y2 = g_dets[i].y2 * 1.6875f;
                    obj.info[i].bbox.score = g_dets[i].score;
                    obj.info[i].classes = g_dets[i].cls;
                }

                cvtdl_service_brush_t brush;

                brush.color.r = 0;
                brush.color.g = 255;
                brush.color.b = 0;

                brush.size = 3;
                CVI_TDL_ObjectDrawRect(&obj, &f, true, brush);
                free(obj.info);
            }
        }

        pthread_mutex_unlock(&g_det_mutex);

        if (CVI_VENC_SendFrame(0, &f, 500) != CVI_SUCCESS) {
            CVI_VPSS_ReleaseChnFrame(0, VPSS_CHN1, &f);
            continue;
        }

        CVI_VPSS_ReleaseChnFrame(0, VPSS_CHN1, &f);

        VENC_CHN_STATUS_S st;

        if (CVI_VENC_QueryStatus(0, &st) != CVI_SUCCESS || st.u32CurPacks == 0)
            continue;

        VENC_STREAM_S s;

        s.pstPack = malloc(st.u32CurPacks * sizeof(VENC_PACK_S));

        if (!s.pstPack)
            continue;

        if (CVI_VENC_GetStream(0, &s, 200) != CVI_SUCCESS) {
            free(s.pstPack);
            continue;
        }

        CVI_RTSP_DATA d = {.blockCnt = s.u32PackCount};

        for (uint32_t i = 0; i < s.u32PackCount; i++) {

            d.dataPtr[i] = s.pstPack[i].pu8Addr + s.pstPack[i].u32Offset;

            d.dataLen[i] = s.pstPack[i].u32Len - s.pstPack[i].u32Offset;
        }

        CVI_RTSP_WriteFrame(g_rtsp, g_ses->video, &d);

        CVI_VENC_ReleaseStream(0, &s);

        free(s.pstPack);
    }

    return NULL;
}

/* ---- YOLO detection ---- */

static void *yolo_thread(void *arg) {

    cvitdl_handle_t hdl = *(cvitdl_handle_t *)arg;

    uint32_t cnt = 0;

    while (g_run) {

        VIDEO_FRAME_INFO_S f;

        if (CVI_VPSS_GetChnFrame(0, VPSS_CHN2, &f, 1000) != CVI_SUCCESS) {
            usleep(10000);
            continue;
        }

        cvtdl_object_t obj = {0};

        struct timeval t0, t1;
        gettimeofday(&t0, NULL);

        CVI_S32 r =
            CVI_TDL_Detection(hdl, &f, CVI_TDL_SUPPORTED_MODEL_YOLOV5, &obj);

        gettimeofday(&t1, NULL);

        // Drain stale frames while detection was running

        VIDEO_FRAME_INFO_S drain;

        while (CVI_VPSS_GetChnFrame(0, VPSS_CHN2, &drain, 0) == CVI_SUCCESS)

            CVI_VPSS_ReleaseChnFrame(0, VPSS_CHN2, &drain);

        if (r != CVI_SUCCESS) {

            if (cnt % 30 == 0) {
                printf("[YOLO #%u] err %#x\n", cnt, r);
                fflush(stdout);
            }

        } else {

            long dt =
                (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;

            /* Copy detections to shared buffer for stream overlay */

            pthread_mutex_lock(&g_det_mutex);

            g_det_count = (obj.size > MAX_DETS) ? MAX_DETS : (int)obj.size;
            if (g_det_count > 0) {
                printf("[YOLO #%u] %.0fms  %d detections: ====", cnt,
                       (float)dt / 1000, g_det_count);
            }
            for (int i = 0; i < g_det_count; i++) {

                g_dets[i].x1 = obj.info[i].bbox.x1;

                g_dets[i].y1 = obj.info[i].bbox.y1;

                g_dets[i].x2 = obj.info[i].bbox.x2;

                g_dets[i].y2 = obj.info[i].bbox.y2;

                g_dets[i].score = obj.info[i].bbox.score;

                g_dets[i].cls = (int)obj.info[i].classes;

                uint32_t c = obj.info[i].classes;
                // printf yolo detection result
                printf("box:%d %.0fms  %s:%.2f", i, (float)dt / 1000,
                       (c < 5) ? g_names[c] : "?", obj.info[i].bbox.score);

                fflush(stdout);
            }
            if (g_det_count > 0)
                printf("=====\n");
            pthread_mutex_unlock(&g_det_mutex);
        }

        cnt++;

        CVI_TDL_Free(&obj);

        CVI_VPSS_ReleaseChnFrame(0, VPSS_CHN2, &f);
    }

    return NULL;
}

/* ---- TDL init ---- */

static CVI_S32 tdl_init(cvitdl_handle_t *h) {

    CVI_S32 r = CVI_TDL_CreateHandle2(h, 1, 0);

    if (r) {
        printf("[TDL] CreateHandle2: %#x\n", r);
        return r;
    }

    CVI_TDL_SetVpssTimeout(*h, 1000);

    InputPreParam pre = CVI_TDL_GetPreParam(*h, CVI_TDL_SUPPORTED_MODEL_YOLOV5);

    for (int i = 0; i < 3; i++) {
        pre.factor[i] = 0.0039216f;
        pre.mean[i] = 0.0f;
    }

    pre.format = PIXEL_FORMAT_RGB_888_PLANAR;

    if ((r = CVI_TDL_SetPreParam(*h, CVI_TDL_SUPPORTED_MODEL_YOLOV5, pre)))
        return r;

    cvtdl_det_algo_param_t a =
        CVI_TDL_GetDetectionAlgoParam(*h, CVI_TDL_SUPPORTED_MODEL_YOLOV5);

    static uint32_t anc[18] = {10, 13, 16,  30,  33, 23,  30,  61,  62,
                               45, 59, 119, 116, 90, 156, 198, 373, 326};

    static uint32_t str[3] = {8, 16, 32};

    a.anchors = anc;
    a.anchor_len = 18;
    a.strides = str;
    a.stride_len = 3;
    a.cls = 5;

    if ((r = CVI_TDL_SetDetectionAlgoParam(*h, CVI_TDL_SUPPORTED_MODEL_YOLOV5,
                                           a)))
        return r;

    r = CVI_TDL_OpenModel(*h, CVI_TDL_SUPPORTED_MODEL_YOLOV5,
                          "gesture_fixed_int8_sym_v2.cvimodel");

    if (r) {
        printf("[TDL] OpenModel: %#x\n", r);
        return r;
    }

    CVI_TDL_SetModelThreshold(*h, CVI_TDL_SUPPORTED_MODEL_YOLOV5, 0.50f);
    CVI_TDL_SetModelNmsThreshold(*h, CVI_TDL_SUPPORTED_MODEL_YOLOV5, 0.3f);

    printf("[TDL] OK (fixed 5-class | thr=0.50)\n");
    return 0;
}

/* VB pool for TDL is handled by CVI_VB_Init (step1_vb.c).
   TDL SDK acquires from system-wide pools by default. */

/* ---- VPSS CHN2 (640x640 NV21) for YOLO ---- */

static CVI_S32 add_vpss_chn2(void) {

    VPSS_CHN_ATTR_S a;
    memset(&a, 0, sizeof(a));

    a.u32Width = 640;
    a.u32Height = 640;
    a.enVideoFormat = VIDEO_FORMAT_LINEAR;

    a.enPixelFormat = PIXEL_FORMAT_NV21;
    a.u32Depth = 1;

    a.stFrameRate.s32SrcFrameRate = 30;
    a.stFrameRate.s32DstFrameRate = 30;

    a.stAspectRatio.enMode = ASPECT_RATIO_NONE;
    a.stNormalize.bEnable = CVI_FALSE;

    a.bFlip = CVI_TRUE;

    CVI_S32 r = CVI_VPSS_SetChnAttr(0, VPSS_CHN2, &a);

    if (r) {
        printf("[VPSS_CHN2] SetChnAttr: %#x\n", r);
        return r;
    }

    r = CVI_VPSS_EnableChn(0, VPSS_CHN2);

    if (r) {
        printf("[VPSS_CHN2] EnableChn: %#x\n", r);
        return r;
    }

    printf("[VPSS_CHN2] OK (640x640)\n");

    return 0;
}

/* ================================================================ */

int main(int argc, char **argv) {

    if (argc >= 2 && !strcmp(argv[1], "reset")) {
        step0_reset(0, 0);
        return 0;
    }

    step0_force_cleanup();

    signal(SIGINT, on_sig);
    signal(SIGTERM, on_sig);

    const char *codec = (argc >= 2) ? argv[1] : "h264";

    printf("=== YOLO Gesture (GC2083 / %s) ===\n", codec);

    /* ---- Pipeline ---- */

    if (CVI_SYS_Init() || CVI_SYS_VI_Open()) {
        printf("[SYS] fail\n");
        return -1;
    }

    if (step1_vb_init()) {
        printf("[VB] fail\n");
        return -1;
    }

    /* VB pool for TDL uses system-wide pools from step1_vb_init */

    if (step3_sensor_init(0) || step4_VI_init(0) || step2_mipi_init(0) ||

        step5_pipe_init(0) || step6_isp_init(0)) {
        printf("[VI] fail\n");
        return -1;
    }

    if (step7_vpss_init(0, 1920, 1080)) {
        printf("[VPSS] fail\n");
        return -1;
    }

    if (step7b_vpss_start(0)) {
        printf("[VPSS] start fail\n");
        return -1;
    }

    if (add_vpss_chn2()) {
        printf("[VPSS+]fail\n");
        return -1;
    }

    if (step8_bind_init(0, 0) || step10_venc_init(0, 1920, 1080) ||

        step10b_venc_bind(0, VPSS_CHN1, 0) ||
        step11_rtsp_create(&g_rtsp, &g_ses, codec) ||

        step11_rtsp_start(g_rtsp, g_ses)) {
        printf("[ENC] fail\n");
        return -1;
    }

    /* ---- TDL ---- */

    cvitdl_handle_t tdl = NULL;

    if (tdl_init(&tdl)) {
        printf("[TDL] fail\n");
        return -1;
    }

    /* ---- Threads ---- */

    g_run = 1;

    pthread_t st, yt;

    pthread_create(&st, NULL, stream_thread, NULL);

    pthread_create(&yt, NULL, yolo_thread, &tdl);

    usleep(300000); /* let pipeline fill */

    CVI_VENC_RequestIDR(0, CVI_TRUE);

    printf("\n=== Ready ===\n  rtsp://<ip>/%s | Ctrl+C to stop\n\n", codec);

    fflush(stdout);

    /* ---- Run until Ctrl+C ---- */

    while (g_run)
        pause();

    /* ---- Shutdown ---- */

    printf("\n=== Shutdown ===\n");

    g_run = 0;
    pthread_join(st, NULL);
    pthread_join(yt, NULL);

    step11_rtsp_stop(g_rtsp, g_ses);
    step10_venc_stop(0);

    {
        MMF_CHN_S s = {.enModId = CVI_ID_VPSS,
                       .s32DevId = 0,
                       .s32ChnId = VPSS_CHN1},

                  d = {.enModId = CVI_ID_VENC, .s32DevId = 0, .s32ChnId = 0};
        CVI_SYS_UnBind(&s, &d);
    }
    step6_isp_stop(0);
    CVI_TDL_DestroyHandle(tdl);
    step0_reset(0, 0);
    return 0;
}
