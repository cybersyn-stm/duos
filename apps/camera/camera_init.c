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
#include "step9_capture.h"

#include <cvi_comm_vb.h>
#include <cvi_comm_venc.h>
#include <cvi_sys.h>
#include <cvi_venc.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ── Globals for streaming thread ── */
static volatile int g_run = 1;
static CVI_RTSP_CTX *g_rtsp_ctx;
static CVI_RTSP_SESSION *g_rtsp_session;

static void sig_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        printf("\n[INFO] Signal received, shutting down...\n");
        g_run = 0;
    }
}

/* ── Streaming thread: VPSS CHN1 → VENC(手动) → RTSP ──
 *
 * 完全对标官方 rtsp_server_video.cpp 的 videoInput 线程模式。
 * ReleaseChnFrame 只在循环末尾调用一次；中间失败路径不释放帧
 *（SendFrame 成功后帧归属 VENC，提前释放会破坏驱动状态导致死锁）。 */
static void *streaming_thread(void *arg)
{
    (void)arg;

    while (g_run) {
        VIDEO_FRAME_INFO_S frame;
        VENC_STREAM_S stream;
        VENC_CHN_STATUS_S stat;
        CVI_S32 ret;

        ret = CVI_VPSS_GetChnFrame(0, VPSS_CHN1, &frame, 200);
        if (ret != CVI_SUCCESS) {
            usleep(50000);
            continue;
        }

        ret = CVI_VENC_SendFrame(0, &frame, 20000);
        if (ret != CVI_SUCCESS) {
            printf("[STREAM] SendFrame err %#x\n", ret);
            CVI_VPSS_ReleaseChnFrame(0, VPSS_CHN1, &frame);
            continue;
        }

        /* SendFrame 成功后帧已移交 VENC，此后不再 ReleaseChnFrame 直到末尾 */

        ret = CVI_VENC_QueryStatus(0, &stat);
        if (ret != CVI_SUCCESS || stat.u32CurPacks == 0)
            goto release_stream_end;

        memset(&stream, 0, sizeof(stream));
        stream.pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S) *
                                                stat.u32CurPacks);
        if (!stream.pstPack)
            goto release_stream_end;

        ret = CVI_VENC_GetStream(0, &stream, -1);
        if (ret != CVI_SUCCESS) {
            free(stream.pstPack);
            stream.pstPack = NULL;
            goto release_stream_end;
        }

        if (g_run) {
            CVI_RTSP_DATA data = {0};
            data.blockCnt = stream.u32PackCount;
            for (unsigned int i = 0; i < stream.u32PackCount; i++) {
                VENC_PACK_S *p = &stream.pstPack[i];
                data.dataPtr[i] = p->pu8Addr + p->u32Offset;
                data.dataLen[i] = p->u32Len - p->u32Offset;
            }
            CVI_RTSP_WriteFrame(g_rtsp_ctx, g_rtsp_session->video, &data);
        }

        CVI_VENC_ReleaseStream(0, &stream);
        free(stream.pstPack);

release_stream_end:
        CVI_VPSS_ReleaseChnFrame(0, VPSS_CHN1, &frame);
    }

    printf("[STREAM] Thread exit\n");
    return NULL;
}

/* ── 按指定名称连拍 100 张 ── */
static void take_sequence(const char *name) {
    char base[128];

    system("mkdir -p photo");
    snprintf(base, sizeof(base), "photo/%s", name);

    printf(">>> Capturing 100 frames → %s_001.yuv … %s_100.yuv\n", base, base);
    int saved = step9_capture_sequence(0, 0, base, 100, &g_run);
    printf(">>> %s: %d/100\n", name, saved);
}

int main(int argc, char **argv) {
    CVI_S32 ret;

    if (argc >= 2 && strcmp(argv[1], "reset") == 0) {
        printf("=== Camera Reset ===\n\n");
        step0_reset(0, 0);
        return 0;
    }

    /* Clean up any leftover state from a previous unclean exit (kill -9
     *
     * etc.)


     */
    step0_force_cleanup();

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    const char *codec = (argc >= 2) ? argv[1] : "h264";

    printf("=== Camera Init (GC2083, 1080p@30fps, RTSP %s) ===\n", codec);

    /*
     * Pipeline order follows sample_vio.c / vi_vo_utils.c EXACTLY:
     *   SYS → VB → Sensor → VI_Dev → MIPI → Pipe → ISP → VI_Chn
     *   → VPSS_Init → VPSS_Start → Bind → VENC → RTSP
     */

    /* System init */
    ret = CVI_SYS_Init();
    if (ret != CVI_SUCCESS) {
        printf("[SYS] Init fail: %#x\n", ret);
        return -1;
    }

    ret = CVI_SYS_VI_Open();
    if (ret != CVI_SUCCESS) {
        printf("[SYS] VI_Open fail: %#x\n", ret);
        return -1;
    }
    printf("[SYS] OK\n");

    /* Step1: VB pool */
    ret = step1_vb_init();
    if (ret != CVI_SUCCESS) {
        printf("[FATAL] Step1 (VB)\n");
        return -1;
    }

    /* Step2: Sensor — SetInit, BusInfo, RegisterCallback, SetImageMode,
     * SetWdrMode per SAMPLE_COMM_VI_StartSensor */
    ret = step3_sensor_init(0);
    if (ret != CVI_SUCCESS) {
        printf("[FATAL] Step2 (Sensor)\n");
        return -1;
    }

    /* Step3: VI Dev — SetDevAttr, EnableDev
     *         per SAMPLE_COMM_VI_StartDev */
    ret = step4_VI_init(0);
    if (ret != CVI_SUCCESS) {
        printf("[FATAL] Step3 (VI Dev)\n");
        return -1;
    }

    /* Step4: MIPI — GetRxAttr, Reset, Clock, Unreset
     *         per SAMPLE_COMM_VI_StartMIPI */
    ret = step2_mipi_init(0);
    if (ret != CVI_SUCCESS) {
        printf("[FATAL] Step4 (MIPI)\n");
        return -1;
    }

    /* Step5: VI Pipe — CreatePipe, StartPipe */
    ret = step5_pipe_init(0);
    if (ret != CVI_SUCCESS) {
        printf("[FATAL] Step5 (Pipe)\n");
        return -1;
    }

    /* Step6: ISP — AE_Register, AWB_Register, SetPubAttr, ISP_Init, ISP_Run
     *         per SAMPLE_COMM_VI_CreateIsp
     *         (VI CHN setup bundled here to match pipe creation) */
    ret = step6_isp_init(0);
    if (ret != CVI_SUCCESS) {
        printf("[FATAL] Step6 (ISP)\n");
        return -1;
    }

    /* Step7: VPSS — CreateGrp, ResetGrp, SetChnAttr, EnableChn
     *         per SAMPLE_COMM_VPSS_Init */
    ret = step7_vpss_init(0, 1920, 1080);
    if (ret != CVI_SUCCESS) {
        printf("[FATAL] Step7 (VPSS Init)\n");
        return -1;
    }

    /* Step8: Bind VI→VPSS — SDK 要求先 Bind 再 StartGrp */
    ret = step8_bind_init(0, 0);
    if (ret != CVI_SUCCESS) {
        printf("[FATAL] Step8 (Bind)\n");
        return -1;
    }

    /* Step9: VPSS StartGrp — 必须在 Bind 之后 */
    ret = step7b_vpss_start(0);
    if (ret != CVI_SUCCESS) {
        printf("[FATAL] Step9 (VPSS Start)\n");
        return -1;
    }

    /* Step10: VENC H.264 encoder */
    ret = step10_venc_init(0, 1920, 1080);
    if (ret != CVI_SUCCESS) {
        printf("[FATAL] Step10 (VENC)\n");
        return -1;
    }

    /* Step11: RTSP create + start */
    ret = step11_rtsp_create(&g_rtsp_ctx, &g_rtsp_session, codec);
    if (ret != CVI_SUCCESS) {
        printf("[FATAL] Step11 (RTSP Create)\n");
        return -1;
    }

    ret = step11_rtsp_start(g_rtsp_ctx, g_rtsp_session);
    if (ret != CVI_SUCCESS) {
        printf("[FATAL] Step11 (RTSP Start)\n");
        return -1;
    }

    /* Start streaming thread */
    pthread_t stream_tid;
    pthread_create(&stream_tid, NULL, streaming_thread, NULL);

    printf("\n=== Ready ===\n");
    printf("  rtsp://<ip>/%s  — live stream\n", codec);
    printf("  <name>           — capture 100 frames → "
           "photo/<name>_001..100.yuv\n");
    printf("  stop / Ctrl+C    — exit\n");
    printf("==================\n\n");

    /* Interactive loop */
    char input[64];
    while (g_run) {
        if (fgets(input, sizeof(input), stdin) == NULL)
            break;

        /* strip trailing newline */
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n')
            input[len - 1] = '\0';

        if (strcmp(input, "stop") == 0 || strcmp(input, "quit") == 0)
            break;

        if (input[0] != '\0') {
            /* 输入名称 → 拍 100 张 */
            take_sequence(input);
        }
    }

    printf("\n=== Shutdown ===\n");

    /* 1. 通知线程退出 */
    g_run = 0;

    /* 2. 关 RTSP（不影响 VENC/VPSS 数据流） */
    step11_rtsp_stop(g_rtsp_ctx, g_rtsp_session);

    /* 3. 停 VENC — StopRecvFrame 会解除 GetStream(-1) 的阻塞 */
    step10_venc_stop(0);

    /* 4. 等待 streaming 线程结束（VENC 已停，GetStream 必然返回） */
    pthread_join(stream_tid, NULL);

    /* Stop ISP thread (must be before system reset) */
    step6_isp_stop(0);

    printf("\n=== Reset ===\n");
    step0_reset(0, 0);

    return 0;
}
