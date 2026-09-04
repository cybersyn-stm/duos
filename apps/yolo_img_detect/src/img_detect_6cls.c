#include "cvi_tdl.h"
#include "cvi_tdl_media.h"
#include "step1_vb.h"
#include <cvi_sys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

static const char *g_names[] = {"one", "two", "three", "four", "five"};

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <input.jpg> [model_path] [conf]\n", argv[0]);
        printf("  input.jpg   - JPEG image to detect\n");
        printf("  model_path  - default: gesture_fixed_int8_sym.cvimodel\n");
        printf("  conf        - confidence threshold (default: 0.5)\n");
        return -1;
    }
    const char *img = argv[1];
    const char *model =
        (argc >= 3) ? argv[2] : "gesture_fixed_int8_sym.cvimodel";
    float conf = (argc >= 4) ? atof(argv[3]) : 0.5f;

    /* ---- Init MMF ---- */
    CVI_SYS_Init();
    step1_vb_init();

    /* ---- Init TDL ---- */
    cvitdl_handle_t h;
    CVI_S32 r = CVI_TDL_CreateHandle2(&h, 1, 0);
    if (r) {
        printf("CreateHandle2: %#x\n", r);
        return -1;
    }

    InputPreParam pre = CVI_TDL_GetPreParam(h, CVI_TDL_SUPPORTED_MODEL_YOLOV5);
    for (int i = 0; i < 3; i++) {
        pre.factor[i] = 0.0039216f;
        pre.mean[i] = 0.0f;
    }
    pre.format = PIXEL_FORMAT_RGB_888_PLANAR;
    if ((r = CVI_TDL_SetPreParam(h, CVI_TDL_SUPPORTED_MODEL_YOLOV5, pre))) {
        printf("SetPreParam: %#x\n", r);
        return -1;
    }

    cvtdl_det_algo_param_t a =
        CVI_TDL_GetDetectionAlgoParam(h, CVI_TDL_SUPPORTED_MODEL_YOLOV5);
    static uint32_t anc[18] = {10, 13, 16,  30,  33, 23,  30,  61,  62,
                               45, 59, 119, 116, 90, 156, 198, 373, 326};
    static uint32_t str[3] = {8, 16, 32};
    a.anchors = anc;
    a.anchor_len = 18;
    a.strides = str;
    a.stride_len = 3;
    a.cls = 5;
    if ((r = CVI_TDL_SetDetectionAlgoParam(h, CVI_TDL_SUPPORTED_MODEL_YOLOV5,
                                           a))) {
        printf("SetAlgoParam: %#x\n", r);
        return -1;
    }

    r = CVI_TDL_OpenModel(h, CVI_TDL_SUPPORTED_MODEL_YOLOV5, model);
    if (r) {
        printf("OpenModel: %#x\n", r);
        return -1;
    }

    CVI_TDL_SetModelThreshold(h, CVI_TDL_SUPPORTED_MODEL_YOLOV5, conf);
    CVI_TDL_SetModelNmsThreshold(h, CVI_TDL_SUPPORTED_MODEL_YOLOV5, 0.5f);

    printf("[CFG] model=%s conf=%.2f classes=%d\n", model, conf, a.cls);

    /* ---- Load JPEG ---- */
    imgprocess_t img_h;
    CVI_TDL_Create_ImageProcessor(&img_h);

    VIDEO_FRAME_INFO_S fdFrame;
    r = CVI_TDL_ReadImage(img_h, img, &fdFrame, PIXEL_FORMAT_RGB_888);
    CVI_TDL_Destroy_ImageProcessor(img_h);
    if (r) {
        printf("ReadImage(%s): %#x\n", img, r);
        return -1;
    }

    /* ---- Infer ---- */
    cvtdl_object_t obj = {0};
    struct timeval t0, t1;
    gettimeofday(&t0, NULL);
    r = CVI_TDL_Detection(h, &fdFrame, CVI_TDL_SUPPORTED_MODEL_YOLOV5, &obj);
    gettimeofday(&t1, NULL);
    long dt = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;

    if (r != CVI_SUCCESS) {
        printf("Detection: %#x\n", r);
    } else {
        printf("Infer: %.1fms | Detected: %u\n", dt / 1000.0f, obj.size);
        for (uint32_t i = 0; i < obj.size && i < 20; i++) {
            uint32_t c = obj.info[i].classes;
            printf("  #%u: %s | conf=%.4f | box=[%.0f,%.0f,%.0f,%.0f]\n", i,
                   (c < 5) ? g_names[c] : "?", obj.info[i].bbox.score,
                   obj.info[i].bbox.x1, obj.info[i].bbox.y1,
                   obj.info[i].bbox.x2, obj.info[i].bbox.y2);
        }
    }

    CVI_TDL_Free(&obj);
    CVI_TDL_ReleaseImage(img_h, &fdFrame);
    CVI_TDL_DestroyHandle(h);
    CVI_SYS_Exit();
    return 0;
}
