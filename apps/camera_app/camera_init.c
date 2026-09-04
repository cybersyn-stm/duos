/**
 * Milk-V Duo GC2083 摄像头 + RTSP 流媒体程序
 *
 * 功能:
 *   - GC2083 sensor 1080p@30fps 采集
 *   - H.264 硬件编码 RTSP 推流 (rtsp://<ip>/h264)
 *   - 交互式拍照 (Enter 键保存快照)
 *
 * 编译: make
 * 运行: ./camera_init
 *
 * 交互:
 *   Enter  → 拍照 (保存到 ./photo/frame_XXXX_640x480.nv12)
 *   "stop" → 退出程序
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <pthread.h>
#include <unistd.h>

/* CVITEK 头文件 */
#include "linux/cvi_type.h"
#include "linux/cvi_common.h"
#include "linux/cvi_comm_video.h"
#include "linux/cvi_comm_vi.h"
#include "linux/cvi_comm_vpss.h"
#include "cvi_comm_vb.h"
#include "linux/cvi_comm_venc.h"
#include "linux/cvi_comm_rc.h"
#include "cvi_ae.h"
#include "cvi_awb.h"
#include "cvi_comm_isp.h"
#include "cvi_isp.h"
#include "cvi_mipi.h"
#include "cvi_sns_ctrl.h"
#include "cvi_sys.h"
#include "cvi_vb.h"
#include "cvi_vi.h"
#include "cvi_vpss.h"
#include "cvi_venc.h"
#include "cvi_rtsp/rtsp.h"

/* Step 模块 */
#include "step7_vpss.h"
#include "step10_venc.h"
#include "step11_rtsp.h"

/* ================================================================
 * 配置
 * ================================================================ */
#define VI_PIPE_NUM     0
#define VI_DEV_NUM      0
#define VI_CHN_NUM      0
#define SENSOR_WIDTH    1920
#define SENSOR_HEIGHT   1080
#define DEFAULT_FPS     30.0f

#define PHOTO_DIR       "./photo"

/* ================================================================
 * 全局变量
 * ================================================================ */
static volatile int g_bRunning = 1;
static pthread_t    g_ispThread;
static int          g_bIspRunning = 0;
static double       g_fTargetFps = DEFAULT_FPS;
static ISP_SNS_OBJ_S *g_pstSnsObj;
static unsigned int g_u32SnapIdx;

/* ================================================================
 * 信号处理
 * ================================================================ */
static void sig_handler(int sig)
{
    if (sig == SIGINT || sig == SIGTERM) {
        printf("\n[INFO] 收到退出信号\n");
        g_bRunning = 0;
    }
}

/* ================================================================
 * 保存单帧 NV12 到文件
 * ================================================================ */
static void save_frame(VIDEO_FRAME_INFO_S *pstFrame, const char *dir)
{
    char path[512];
    FILE *fp;
    size_t image_size;
    void *vir_addr;
    int i;

    image_size = pstFrame->stVFrame.u32Length[0]
               + pstFrame->stVFrame.u32Length[1]
               + pstFrame->stVFrame.u32Length[2];

    vir_addr = CVI_SYS_Mmap(pstFrame->stVFrame.u64PhyAddr[0], image_size);
    if (!vir_addr) {
        fprintf(stderr, "[ERROR] CVI_SYS_Mmap 失败\n");
        return;
    }

    CVI_SYS_IonInvalidateCache(pstFrame->stVFrame.u64PhyAddr[0],
                               vir_addr, image_size);

    size_t plane_offset = 0;
    for (i = 0; i < 3; i++) {
        if (pstFrame->stVFrame.u32Length[i] != 0) {
            pstFrame->stVFrame.pu8VirAddr[i] = (CVI_U8 *)vir_addr + plane_offset;
            plane_offset += pstFrame->stVFrame.u32Length[i];
        }
    }

    mkdir(dir, 0755);

    snprintf(path, sizeof(path), "%s/frame_%04d_%dx%d.nv12",
             dir, g_u32SnapIdx,
             (int)pstFrame->stVFrame.u32Width,
             (int)pstFrame->stVFrame.u32Height);

    fp = fopen(path, "wb");
    if (!fp) {
        fprintf(stderr, "[ERROR] 无法创建 %s\n", path);
        CVI_SYS_Munmap(vir_addr, image_size);
        return;
    }

    CVI_U32 u32LumaSize   = pstFrame->stVFrame.u32Stride[0] * pstFrame->stVFrame.u32Height;
    CVI_U32 u32ChromaSize = pstFrame->stVFrame.u32Stride[1] * pstFrame->stVFrame.u32Height / 2;

    if (pstFrame->stVFrame.pu8VirAddr[0])
        fwrite(pstFrame->stVFrame.pu8VirAddr[0], u32LumaSize, 1, fp);
    if (pstFrame->stVFrame.pu8VirAddr[1])
        fwrite(pstFrame->stVFrame.pu8VirAddr[1], u32ChromaSize, 1, fp);

    fclose(fp);
    CVI_SYS_Munmap(vir_addr, image_size);

    printf("[CAPTURE] #%04d %s (%dx%d)\n",
           g_u32SnapIdx, path,
           (int)pstFrame->stVFrame.u32Width,
           (int)pstFrame->stVFrame.u32Height);
    g_u32SnapIdx++;
}

/* ================================================================
 * 拍照：从 VPSS CHN0 取一帧
 * ================================================================ */
static void take_snapshot(void)
{
    VIDEO_FRAME_INFO_S stFrame;
    CVI_S32 s32Ret;

    memset(&stFrame, 0, sizeof(stFrame));
    s32Ret = CVI_VPSS_GetChnFrame(VPSS_GRP_NUM, VPSS_CHN_SNAP, &stFrame, 500);
    if (s32Ret != CVI_SUCCESS) {
        fprintf(stderr, "[ERROR] GetChnFrame(CHN0): 0x%x\n", s32Ret);
        return;
    }

    save_frame(&stFrame, PHOTO_DIR);
    CVI_VPSS_ReleaseChnFrame(VPSS_GRP_NUM, VPSS_CHN_SNAP, &stFrame);
}

/* ================================================================
 * Sensor 初始化
 * ================================================================ */
static int sensor_init(VI_PIPE ViPipe)
{
    ISP_SNS_COMMBUS_U unBusInfo;
    ISP_INIT_ATTR_S   stInitAttr;
    ALG_LIB_S         stAeLib, stAwbLib;
    CVI_S32 s32Ret;

    printf("[SENSOR] 初始化 GC2083 sensor...\n");

    g_pstSnsObj = &stSnsGc2083_Obj;

    /* I2C 总线 */
    unBusInfo.s8I2cDev = 2;
    s32Ret = g_pstSnsObj->pfnSetBusInfo(ViPipe, unBusInfo);
    if (s32Ret != CVI_SUCCESS) {
        fprintf(stderr, "[ERROR] pfnSetBusInfo: 0x%x\n", s32Ret);
        return -1;
    }

    if (g_pstSnsObj->pfnPatchI2cAddr)
        g_pstSnsObj->pfnPatchI2cAddr(0x37);

    /* MIPI RX 属性 */
    RX_INIT_ATTR_S stRxInitAttr;
    memset(&stRxInitAttr, 0, sizeof(stRxInitAttr));
    stRxInitAttr.MipiDev       = 0;
    stRxInitAttr.as16LaneId[0] = 1;
    stRxInitAttr.as16LaneId[1] = 0;
    stRxInitAttr.as16LaneId[2] = 2;
    stRxInitAttr.as16LaneId[3] = -1;
    stRxInitAttr.as16LaneId[4] = -1;
    stRxInitAttr.stMclkAttr.bMclkEn = CVI_FALSE;

    if (g_pstSnsObj->pfnPatchRxAttr)
        g_pstSnsObj->pfnPatchRxAttr(&stRxInitAttr);

    /* AE/AWB */
    stAeLib.s32Id  = ViPipe;
    stAwbLib.s32Id = ViPipe;
    strncpy(stAeLib.acLibName, "ae", sizeof(stAeLib.acLibName));
    strncpy(stAwbLib.acLibName, "awb", sizeof(stAwbLib.acLibName));

    /* Init attr */
    memset(&stInitAttr, 0, sizeof(stInitAttr));
    stInitAttr.u32LinesPer500ms = (CVI_U32)(DEFAULT_FPS * 500 / 1000);
    stInitAttr.enGainMode       = SNS_GAIN_MODE_SHARE;

    if (g_pstSnsObj->pfnSetInit)
        g_pstSnsObj->pfnSetInit(ViPipe, &stInitAttr);

    /* 注册 sensor 回调到 ISP/AE/AWB */
    if (g_pstSnsObj->pfnRegisterCallback)
        g_pstSnsObj->pfnRegisterCallback(ViPipe, &stAeLib, &stAwbLib);

    printf("[SENSOR] GC2083 初始化完成\n");
    return 0;
}

/* ================================================================
 * VI 设备初始化
 * ================================================================ */
static int vi_dev_init(void)
{
    VI_DEV_ATTR_S        stDevAttr;
    SNS_COMBO_DEV_ATTR_S stRxAttr;
    CVI_S32 s32Ret;

    printf("[VI] 初始化 VI 设备...\n");

    memset(&stRxAttr, 0, sizeof(stRxAttr));
    if (g_pstSnsObj->pfnGetRxAttr)
        g_pstSnsObj->pfnGetRxAttr(VI_PIPE_NUM, &stRxAttr);

    memset(&stDevAttr, 0, sizeof(stDevAttr));
    stDevAttr.enIntfMode      = VI_MODE_MIPI;
    stDevAttr.enWorkMode      = VI_WORK_MODE_1Multiplex;
    stDevAttr.enScanMode      = VI_SCAN_PROGRESSIVE;
    stDevAttr.as32AdChnId[0]  = -1;
    stDevAttr.as32AdChnId[1]  = -1;
    stDevAttr.as32AdChnId[2]  = -1;
    stDevAttr.as32AdChnId[3]  = -1;
    stDevAttr.enInputDataType = VI_DATA_TYPE_RGB;
    stDevAttr.enBayerFormat   = BAYER_FORMAT_BG;
    stDevAttr.chn_num         = 1;
    stDevAttr.stSize.u32Width  = stRxAttr.img_size.width;
    stDevAttr.stSize.u32Height = stRxAttr.img_size.height;
    stDevAttr.enDataSeq        = VI_DATA_SEQ_YUYV;

    s32Ret = CVI_VI_SetDevNum(1);
    if (s32Ret != CVI_SUCCESS) { fprintf(stderr, "[ERROR] SetDevNum: 0x%x\n", s32Ret); return -1; }

    s32Ret = CVI_VI_SetDevAttr(VI_DEV_NUM, &stDevAttr);
    if (s32Ret != CVI_SUCCESS) { fprintf(stderr, "[ERROR] SetDevAttr: 0x%x\n", s32Ret); return -1; }

    s32Ret = CVI_VI_EnableDev(VI_DEV_NUM);
    if (s32Ret != CVI_SUCCESS) { fprintf(stderr, "[ERROR] EnableDev: 0x%x\n", s32Ret); return -1; }

    printf("[VI] 设备初始化完成\n");
    return 0;
}

/* ================================================================
 * MIPI 初始化
 * ================================================================ */
static int mipi_init(void)
{
    SNS_COMBO_DEV_ATTR_S stRxAttr;
    CVI_S32 s32Ret;

    printf("[MIPI] 初始化 MIPI...\n");

    memset(&stRxAttr, 0, sizeof(stRxAttr));
    if (g_pstSnsObj->pfnGetRxAttr)
        g_pstSnsObj->pfnGetRxAttr(VI_PIPE_NUM, &stRxAttr);

    s32Ret = CVI_MIPI_SetMipiAttr(VI_PIPE_NUM, (CVI_VOID *)&stRxAttr);
    if (s32Ret != CVI_SUCCESS) { fprintf(stderr, "[ERROR] SetMipiAttr: 0x%x\n", s32Ret); return -1; }

    s32Ret = CVI_MIPI_SetSensorClock(0, 1);
    if (s32Ret != CVI_SUCCESS) { fprintf(stderr, "[ERROR] SetSensorClock: 0x%x\n", s32Ret); return -1; }
    usleep(20);

    s32Ret = CVI_MIPI_SetSensorReset(0, 0);
    if (s32Ret != CVI_SUCCESS) { fprintf(stderr, "[ERROR] SetSensorReset: 0x%x\n", s32Ret); return -1; }
    usleep(10000);

    printf("[MIPI] 初始化完成\n");
    return 0;
}

/* ================================================================
 * VI Pipe
 * ================================================================ */
static int vi_pipe_init(void)
{
    VI_DEV_BIND_PIPE_S stBindPipe;
    VI_PIPE_ATTR_S     stPipeAttr;
    CVI_S32 s32Ret;

    printf("[VI] 创建 Pipe...\n");

    stBindPipe.u32Num    = 1;
    stBindPipe.PipeId[0] = VI_PIPE_NUM;
    s32Ret = CVI_VI_SetDevBindPipe(VI_DEV_NUM, &stBindPipe);
    if (s32Ret != CVI_SUCCESS) { fprintf(stderr, "[ERROR] SetDevBindPipe: 0x%x\n", s32Ret); return -1; }

    memset(&stPipeAttr, 0, sizeof(stPipeAttr));
    stPipeAttr.u32MaxW    = SENSOR_WIDTH;
    stPipeAttr.u32MaxH    = SENSOR_HEIGHT;
    stPipeAttr.enPixFmt   = PIXEL_FORMAT_RGB_BAYER_12BPP;
    stPipeAttr.enBitWidth = DATA_BITWIDTH_12;
    stPipeAttr.bYuvBypassPath = CVI_FALSE;
    stPipeAttr.bNrEn      = CVI_TRUE;
    stPipeAttr.stFrameRate.s32SrcFrameRate = -1;
    stPipeAttr.stFrameRate.s32DstFrameRate = -1;

    s32Ret = CVI_VI_CreatePipe(VI_PIPE_NUM, &stPipeAttr);
    if (s32Ret != CVI_SUCCESS) { fprintf(stderr, "[ERROR] CreatePipe: 0x%x\n", s32Ret); return -1; }

    s32Ret = CVI_VI_StartPipe(VI_PIPE_NUM);
    if (s32Ret != CVI_SUCCESS) { fprintf(stderr, "[ERROR] StartPipe: 0x%x\n", s32Ret); return -1; }

    printf("[VI] Pipe 创建完成\n");
    return 0;
}

/* ================================================================
 * VI Channel
 * ================================================================ */
static int vi_chn_init(void)
{
    VI_CHN_ATTR_S stChnAttr;
    CVI_S32 s32Ret;

    printf("[VI] 创建 Channel...\n");

    memset(&stChnAttr, 0, sizeof(stChnAttr));
    stChnAttr.stSize.u32Width   = SENSOR_WIDTH;
    stChnAttr.stSize.u32Height  = SENSOR_HEIGHT;
    stChnAttr.enPixelFormat     = PIXEL_FORMAT_YUV_PLANAR_420;
    stChnAttr.enDynamicRange    = DYNAMIC_RANGE_SDR8;
    stChnAttr.enVideoFormat     = VIDEO_FORMAT_LINEAR;
    stChnAttr.enCompressMode    = COMPRESS_MODE_NONE;
    stChnAttr.stFrameRate.s32SrcFrameRate = -1;
    stChnAttr.stFrameRate.s32DstFrameRate = -1;

    s32Ret = CVI_VI_SetChnAttr(VI_PIPE_NUM, VI_CHN_NUM, &stChnAttr);
    if (s32Ret != CVI_SUCCESS) { fprintf(stderr, "[ERROR] SetChnAttr: 0x%x\n", s32Ret); return -1; }

    s32Ret = CVI_VI_EnableChn(VI_PIPE_NUM, VI_CHN_NUM);
    if (s32Ret != CVI_SUCCESS) { fprintf(stderr, "[ERROR] EnableChn: 0x%x\n", s32Ret); return -1; }

    printf("[VI] Channel 创建完成\n");
    return 0;
}

/* ================================================================
 * ISP 线程
 * ================================================================ */
static void *isp_thread(void *arg)
{
    VI_PIPE ViPipe = *(VI_PIPE *)arg;
    prctl(PR_SET_NAME, "ISP_RUN", 0, 0, 0);

    CVI_S32 s32Ret = CVI_ISP_Run(ViPipe);
    if (s32Ret != CVI_SUCCESS)
        fprintf(stderr, "[ERROR] CVI_ISP_Run: 0x%x\n", s32Ret);

    return NULL;
}

/* ================================================================
 * ISP 初始化
 * ================================================================ */
static int isp_init(VI_PIPE ViPipe)
{
    ISP_PUB_ATTR_S  stPubAttr;
    ISP_BIND_ATTR_S stBindAttr;
    ALG_LIB_S       stAeLib, stAwbLib;
    CVI_S32 s32Ret;

    printf("[ISP] 初始化 ISP...\n");

    /* AE */
    stAeLib.s32Id = ViPipe;
    strncpy(stAeLib.acLibName, CVI_AE_LIB_NAME, sizeof(stAeLib.acLibName));
    CVI_AE_Register(ViPipe, &stAeLib);

    /* AWB */
    stAwbLib.s32Id = ViPipe;
    strncpy(stAwbLib.acLibName, CVI_AWB_LIB_NAME, sizeof(stAwbLib.acLibName));
    CVI_AWB_Register(ViPipe, &stAwbLib);

    /* Bind AE/AWB */
    memset(&stBindAttr, 0, sizeof(stBindAttr));
    stBindAttr.sensorId          = 0;
    stBindAttr.stAeLib.s32Id     = ViPipe;
    strncpy(stBindAttr.stAeLib.acLibName, CVI_AE_LIB_NAME,
            sizeof(stBindAttr.stAeLib.acLibName));
    stBindAttr.stAwbLib.s32Id    = ViPipe;
    strncpy(stBindAttr.stAwbLib.acLibName, CVI_AWB_LIB_NAME,
            sizeof(stBindAttr.stAwbLib.acLibName));

    s32Ret = CVI_ISP_SetBindAttr(ViPipe, &stBindAttr);
    if (s32Ret != CVI_SUCCESS) { fprintf(stderr, "[ERROR] SetBindAttr: 0x%x\n", s32Ret); return -1; }

    /* MemInit */
    s32Ret = CVI_ISP_MemInit(ViPipe);
    if (s32Ret != CVI_SUCCESS) { fprintf(stderr, "[ERROR] MemInit: 0x%x\n", s32Ret); return -1; }

    /* PubAttr */
    memset(&stPubAttr, 0, sizeof(stPubAttr));
    stPubAttr.stWndRect.s32X      = 0;
    stPubAttr.stWndRect.s32Y      = 0;
    stPubAttr.stWndRect.u32Width  = SENSOR_WIDTH;
    stPubAttr.stWndRect.u32Height = SENSOR_HEIGHT;
    stPubAttr.stSnsSize.u32Width  = SENSOR_WIDTH;
    stPubAttr.stSnsSize.u32Height = SENSOR_HEIGHT;
    stPubAttr.f32FrameRate        = g_fTargetFps;
    stPubAttr.enBayer             = BAYER_BGGR;
    stPubAttr.enWDRMode           = WDR_MODE_NONE;
    stPubAttr.u8SnsMode           = 0;

    s32Ret = CVI_ISP_SetPubAttr(ViPipe, &stPubAttr);
    if (s32Ret != CVI_SUCCESS) { fprintf(stderr, "[ERROR] SetPubAttr: 0x%x\n", s32Ret); return -1; }

    /* Init */
    s32Ret = CVI_ISP_Init(ViPipe);
    if (s32Ret != CVI_SUCCESS) { fprintf(stderr, "[ERROR] Init: 0x%x\n", s32Ret); return -1; }

    /* ISP 线程 */
    if (pthread_create(&g_ispThread, NULL, isp_thread, &ViPipe) != 0) {
        fprintf(stderr, "[ERROR] pthread_create (ISP) 失败\n");
        return -1;
    }
    g_bIspRunning = 1;

    printf("[ISP] 初始化完成\n");
    return 0;
}

/* ================================================================
 * VB 初始化
 * ================================================================ */
static int vb_init(void)
{
    VB_CONFIG_S stVbConf;
    CVI_S32 s32Ret;

    printf("[VB] 初始化 Video Buffer...\n");

    memset(&stVbConf, 0, sizeof(stVbConf));
    stVbConf.u32MaxPoolCnt = 1;
    stVbConf.astCommPool[0].u32BlkSize =
        SENSOR_WIDTH * SENSOR_HEIGHT * 3 / 2;
    stVbConf.astCommPool[0].u32BlkCnt = 10;

    s32Ret = CVI_VB_SetConfig(&stVbConf);
    if (s32Ret != CVI_SUCCESS)
        fprintf(stderr, "[WARN] VB_SetConfig: 0x%x, 使用默认\n", s32Ret);

    s32Ret = CVI_VB_Init();
    if (s32Ret != CVI_SUCCESS) { fprintf(stderr, "[ERROR] VB_Init: 0x%x\n", s32Ret); return -1; }

    printf("[VB] 初始化完成\n");
    return 0;
}

/* ================================================================
 * RTSP 流传输线程
 * ================================================================ */
static void *streaming_thread(void *arg)
{
    (void)arg;
    CVI_RTSP_CTX     *rtspCtx = step11_rtsp_get_ctx();
    CVI_RTSP_SESSION *session = step11_rtsp_get_session();

    if (!rtspCtx || !session) {
        fprintf(stderr, "[STREAM] RTSP 未就绪\n");
        return NULL;
    }

    prctl(PR_SET_NAME, "RTSP_STREAM", 0, 0, 0);
    printf("[STREAM] 开始推送流...\n");

    while (g_bRunning) {
        VIDEO_FRAME_INFO_S stFrame;
        CVI_S32 s32Ret;

        /* 1. 从 VPSS CHN1 取帧 */
        s32Ret = CVI_VPSS_GetChnFrame(VPSS_GRP_NUM, VPSS_CHN_STREAM,
                                       &stFrame, 1000);
        if (s32Ret != CVI_SUCCESS) {
            if (s32Ret == (CVI_S32)0xA0078010)  /* BUF_EMPTY */
                continue;
            fprintf(stderr, "[STREAM] GetChnFrame(CHN1): 0x%x\n", s32Ret);
            continue;
        }

        /* 2. 送 VENC 编码 */
        s32Ret = CVI_VENC_SendFrame(VENC_CHN_NUM, &stFrame, 20000);
        if (s32Ret != CVI_SUCCESS) {
            fprintf(stderr, "[STREAM] SendFrame: 0x%x\n", s32Ret);
            CVI_VPSS_ReleaseChnFrame(VPSS_GRP_NUM, VPSS_CHN_STREAM, &stFrame);
            continue;
        }

        /* 3. 释放 VPSS 帧（SendFrame 已拷贝数据） */
        CVI_VPSS_ReleaseChnFrame(VPSS_GRP_NUM, VPSS_CHN_STREAM, &stFrame);

        /* 4. 查询编码状态 */
        VENC_CHN_STATUS_S stStat;
        s32Ret = CVI_VENC_QueryStatus(VENC_CHN_NUM, &stStat);
        if (s32Ret != CVI_SUCCESS || stStat.u32CurPacks == 0)
            continue;

        /* 5. 获取编码流 */
        VENC_STREAM_S stStream;
        memset(&stStream, 0, sizeof(stStream));
        stStream.pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
        if (!stStream.pstPack)
            continue;

        s32Ret = CVI_VENC_GetStream(VENC_CHN_NUM, &stStream, -1);
        if (s32Ret != CVI_SUCCESS) {
            free(stStream.pstPack);
            continue;
        }

        /* 6. 写入 RTSP */
        CVI_RTSP_DATA data;
        memset(&data, 0, sizeof(data));
        data.blockCnt = stStream.u32PackCount;
        for (unsigned int i = 0; i < stStream.u32PackCount; i++) {
            VENC_PACK_S *ppack = &stStream.pstPack[i];
            data.dataPtr[i] = ppack->pu8Addr + ppack->u32Offset;
            data.dataLen[i] = ppack->u32Len - ppack->u32Offset;
        }

        CVI_RTSP_WriteFrame(rtspCtx, session->video, &data);

        /* 7. 释放流 */
        CVI_VENC_ReleaseStream(VENC_CHN_NUM, &stStream);
        free(stStream.pstPack);
    }

    printf("[STREAM] 流推送线程退出\n");
    return NULL;
}

/* ================================================================
 * 清理
 * ================================================================ */
static void cleanup(void)
{
    printf("\n[CLEANUP] 释放资源...\n");

    /* 1. 解绑 VI→VPSS */
    MMF_CHN_S stSrcChn, stDestChn;
    stSrcChn.enModId  = CVI_ID_VI;
    stSrcChn.s32DevId = VI_PIPE_NUM;
    stSrcChn.s32ChnId = 0;
    stDestChn.enModId  = CVI_ID_VPSS;
    stDestChn.s32DevId = VPSS_GRP_NUM;
    stDestChn.s32ChnId = 0;
    CVI_SYS_UnBind(&stSrcChn, &stDestChn);

    /* 2. 停止 RTSP + VENC + VPSS */
    step11_rtsp_stop();
    step10_venc_stop(VENC_CHN_NUM);
    step7_vpss_cleanup(VPSS_GRP_NUM);

    /* 3. VI + ISP */
    CVI_VI_DisableChn(VI_PIPE_NUM, VI_CHN_NUM);
    CVI_VI_StopPipe(VI_PIPE_NUM);
    CVI_VI_DestroyPipe(VI_PIPE_NUM);
    CVI_VI_DisableDev(VI_DEV_NUM);
    CVI_VI_SetDevNum(0);

    /* 4. Sensor 反注册 */
    if (g_pstSnsObj && g_pstSnsObj->pfnUnRegisterCallback) {
        ALG_LIB_S stAeLib, stAwbLib;
        stAeLib.s32Id  = VI_PIPE_NUM;
        stAwbLib.s32Id = VI_PIPE_NUM;
        strncpy(stAeLib.acLibName, "ae", sizeof(stAeLib.acLibName));
        strncpy(stAwbLib.acLibName, "awb", sizeof(stAwbLib.acLibName));
        g_pstSnsObj->pfnUnRegisterCallback(VI_PIPE_NUM, &stAeLib, &stAwbLib);
    }

    CVI_ISP_Exit(VI_PIPE_NUM);

    if (g_bIspRunning) {
        pthread_join(g_ispThread, NULL);
        g_bIspRunning = 0;
    }

    CVI_VB_Exit();
    CVI_SYS_Exit();

    printf("[CLEANUP] 完成\n");
}

/* ================================================================
 * 主函数
 * ================================================================ */
int main(int argc, char *argv[])
{
    int ret;
    pthread_t streamTid;
    (void)argc; (void)argv;

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGPIPE, SIG_IGN);

    printf("===== Milk-V Duo GC2083 Camera + RTSP =====\n\n");

    /* 系统初始化 */
    ret = CVI_SYS_Init();
    if (ret != CVI_SUCCESS) {
        fprintf(stderr, "[ERROR] CVI_SYS_Init: 0x%x\n", ret);
        return -1;
    }
    printf("[SYS] OK\n");

    /* Step 1-6: VB → Sensor → VI → MIPI → Pipe → ISP → VI CHN */
    if (vb_init()             != 0) goto err;
    printf("[VB] OK\n");
    if (sensor_init(VI_PIPE_NUM) != 0) goto err;
    printf("[SENSOR] OK\n");
    if (vi_dev_init()         != 0) goto err;
    printf("[VI DEV] OK\n");
    if (mipi_init()           != 0) goto err;
    printf("[MIPI] OK\n");
    if (vi_pipe_init()        != 0) goto err;
    printf("[VI PIPE] OK\n");
    if (isp_init(VI_PIPE_NUM) != 0) goto err;
    printf("[ISP] OK\n");
    if (vi_chn_init()         != 0) goto err;
    printf("[VI CHN] OK\n");

    /* Step 7: VPSS 双通道 */
    if (step7_vpss_init(VPSS_GRP_NUM) != 0) goto err;
    if (step7c_bind_vi_to_vpss() != 0) {
        fprintf(stderr, "[ERROR] bind VI→VPSS\n"); goto err;
    }
    printf("[BIND VI→VPSS] OK\n");
    if (step7b_vpss_start(VPSS_GRP_NUM) != 0) goto err;

    /* Step 10: VENC H.264 */
    if (step10_venc_init(VENC_CHN_NUM) != 0) goto err;
    if (step10b_venc_bind(VENC_CHN_NUM) != 0) {
        fprintf(stderr, "[ERROR] bind VPSS→VENC\n"); goto err;
    }
    printf("[BIND VPSS→VENC] OK\n");

    /* Step 11: RTSP */
    if (step11_rtsp_create() != 0) goto err;
    if (step11_rtsp_start()  != 0) goto err;

    /* 启动流传输线程 */
    pthread_create(&streamTid, NULL, streaming_thread, NULL);

    /* ─── 交互式主循环 ─── */
    printf("\n========================================\n");
    printf("  RTSP: rtsp://192.168.42.1/h264\n");
    printf("  命令: Enter = 拍照  \"stop\" = 退出\n");
    printf("========================================\n\n");

    char input[256];
    while (g_bRunning) {
        printf("> ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin))
            break;

        /* 去掉换行 */
        size_t len = strlen(input);
        while (len > 0 && (input[len - 1] == '\n' || input[len - 1] == '\r'))
            input[--len] = '\0';

        if (len == 0) {
            /* Enter 空行 = 拍照 */
            take_snapshot();
        } else if (strcmp(input, "stop") == 0 || strcmp(input, "quit") == 0) {
            printf("[INFO] 退出...\n");
            g_bRunning = 0;
        } else {
            printf("[INFO] 未知命令: '%s'\n", input);
        }
    }

    /* 等待流传输线程结束 */
    pthread_join(streamTid, NULL);

err:
    cleanup();
    return 0;
}
