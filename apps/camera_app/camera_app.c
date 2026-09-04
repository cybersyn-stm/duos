/**
 * Milk-V Duo GC2083 摄像头采集程序 (cv181x SDK)
 *
 * 编译：make
 * 运行：./camera_app                  # 连续模式，打印时间戳
 *       ./camera_app -o ./frames/     # 保存帧到目录
 *       ./camera_app -n 100           # 采集 100 帧后退出
 *       ./camera_app -f 15            # 15 fps
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/prctl.h>
#include <pthread.h>
#include <unistd.h>

/* CVITEK Middleware 头文件 */
#include "linux/cvi_type.h"
#include "linux/cvi_common.h"
#include "linux/cvi_comm_video.h"
#include "linux/cvi_comm_vi.h"
#include "linux/cvi_comm_vpss.h"
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

/* ================================================================
 * 配置
 * ================================================================ */
#define VI_PIPE_NUM     0
#define VI_DEV_NUM      0
#define VI_CHN_NUM      0
#define VPSS_GRP_NUM    0
#define VPSS_CHN_NUM    0

#define SENSOR_WIDTH    1920
#define SENSOR_HEIGHT   1080
#define DEFAULT_FPS     30.0f

#define OUTPUT_WIDTH    640
#define OUTPUT_HEIGHT   480

/* ================================================================
 * 全局变量
 * ================================================================ */
static volatile int g_bRunning = 1;
static char         g_acOutputDir[256];
static unsigned int g_u32MaxFrames;
static unsigned int g_u32FrameCount;
static double       g_fTargetFps = DEFAULT_FPS;
static pthread_t    g_ispThread;
static int          g_bIspRunning = 0;

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
    static int idx;
    char path[512];
    FILE *fp;
    size_t image_size, plane_offset;
    void *vir_addr;
    int i;

    /* NV12: 3 个 plane (Y + UV interleaved) */
    image_size = pstFrame->stVFrame.u32Length[0]
               + pstFrame->stVFrame.u32Length[1]
               + pstFrame->stVFrame.u32Length[2];

    /* 映射物理内存到用户空间 */
    vir_addr = CVI_SYS_Mmap(pstFrame->stVFrame.u64PhyAddr[0], image_size);
    if (!vir_addr) {
        fprintf(stderr, "[ERROR] CVI_SYS_Mmap 失败\n");
        return;
    }

    /* 刷新缓存以读取最新帧数据 */
    CVI_SYS_IonInvalidateCache(pstFrame->stVFrame.u64PhyAddr[0],
                               vir_addr, image_size);

    /* 设置各 plane 虚拟地址 */
    plane_offset = 0;
    for (i = 0; i < 3; i++) {
        if (pstFrame->stVFrame.u32Length[i] != 0) {
            pstFrame->stVFrame.pu8VirAddr[i] = vir_addr + plane_offset;
            plane_offset += pstFrame->stVFrame.u32Length[i];
        }
    }

    snprintf(path, sizeof(path), "%s/frame_%04d_%dx%d.nv12",
             dir, idx,
             (int)pstFrame->stVFrame.u32Width,
             (int)pstFrame->stVFrame.u32Height);

    fp = fopen(path, "wb");
    if (!fp) {
        fprintf(stderr, "[ERROR] 无法创建 %s\n", path);
        CVI_SYS_Munmap(vir_addr, image_size);
        return;
    }

    /* 写入各 plane 数据 (对齐 SDK sample: stride * height) */
    CVI_U32 u32LumaSize   = pstFrame->stVFrame.u32Stride[0] * pstFrame->stVFrame.u32Height;
    CVI_U32 u32ChromaSize = pstFrame->stVFrame.u32Stride[1] * pstFrame->stVFrame.u32Height / 2;

    printf("[SAVE] w=%d h=%d stride[0]=%d stride[1]=%d len[0]=%d len[1]=%d luma=%u chroma=%u\n",
           pstFrame->stVFrame.u32Width, pstFrame->stVFrame.u32Height,
           pstFrame->stVFrame.u32Stride[0], pstFrame->stVFrame.u32Stride[1],
           pstFrame->stVFrame.u32Length[0], pstFrame->stVFrame.u32Length[1],
           u32LumaSize, u32ChromaSize);

    if (pstFrame->stVFrame.pu8VirAddr[0])
        fwrite(pstFrame->stVFrame.pu8VirAddr[0], u32LumaSize, 1, fp);
    if (pstFrame->stVFrame.pu8VirAddr[1])
        fwrite(pstFrame->stVFrame.pu8VirAddr[1], u32ChromaSize, 1, fp);

    fclose(fp);
    printf("[SAVE] %s (%zu bytes)\n", path, image_size);

    /* 解除映射 */
    CVI_SYS_Munmap(vir_addr, image_size);
    idx++;
}

/* ================================================================
 * 全局 sensor 对象指针
 * ================================================================ */
static ISP_SNS_OBJ_S *g_pstSnsObj;

/* ================================================================
 * Sensor 初始化: 注册 sensor 回调到 ISP/AE/AWB
 * ================================================================ */
static int sensor_init(VI_PIPE ViPipe)
{
    ISP_SNS_COMMBUS_U unBusInfo;
    RX_INIT_ATTR_S    stRxInitAttr;
    ISP_INIT_ATTR_S   stInitAttr;
    ALG_LIB_S         stAeLib, stAwbLib;
    CVI_S32 s32Ret;

    printf("[SENSOR] 初始化 GC2083 sensor...\n");

    /* 使用 GC2083 sensor 对象（来自 libsns_gc2083.so） */
    g_pstSnsObj = &stSnsGc2083_Obj;

    /* 设置 I2C 总线：bus_id=2（来自 sensor_cfg.ini） */
    unBusInfo.s8I2cDev = 2;
    s32Ret = g_pstSnsObj->pfnSetBusInfo(ViPipe, unBusInfo);
    if (s32Ret != CVI_SUCCESS) {
        fprintf(stderr, "[ERROR] pfnSetBusInfo: 0x%x\n", s32Ret);
        return -1;
    }

    /* 设置 I2C 地址：0x37（来自 sensor_cfg.ini） */
    if (g_pstSnsObj->pfnPatchI2cAddr)
        g_pstSnsObj->pfnPatchI2cAddr(0x37);

    /* 设置 MIPI RX 属性：MIPI dev=0, lanes=[1,0,2,-1,-1], no pn_swap */
    memset(&stRxInitAttr, 0, sizeof(stRxInitAttr));
    stRxInitAttr.MipiDev           = 0;
    stRxInitAttr.as16LaneId[0]     = 1;
    stRxInitAttr.as16LaneId[1]     = 0;
    stRxInitAttr.as16LaneId[2]     = 2;
    stRxInitAttr.as16LaneId[3]     = -1;
    stRxInitAttr.as16LaneId[4]     = -1;
    stRxInitAttr.as8PNSwap[0]      = 0;
    stRxInitAttr.as8PNSwap[1]      = 0;
    stRxInitAttr.as8PNSwap[2]      = 0;
    stRxInitAttr.as8PNSwap[3]      = 0;
    stRxInitAttr.as8PNSwap[4]      = 0;
    stRxInitAttr.stMclkAttr.bMclkEn = CVI_FALSE;

    if (g_pstSnsObj->pfnPatchRxAttr) {
        s32Ret = g_pstSnsObj->pfnPatchRxAttr(&stRxInitAttr);
        if (s32Ret != CVI_SUCCESS) {
            fprintf(stderr, "[ERROR] pfnPatchRxAttr: 0x%x\n", s32Ret);
            return -1;
        }
    }

    /* AE/AWB 算法库 */
    stAeLib.s32Id  = ViPipe;
    stAwbLib.s32Id = ViPipe;
    strncpy(stAeLib.acLibName, "ae", sizeof(stAeLib.acLibName));
    strncpy(stAwbLib.acLibName, "awb", sizeof(stAwbLib.acLibName));

    /* 设置初始化属性 */
    memset(&stInitAttr, 0, sizeof(stInitAttr));
    stInitAttr.u32LinesPer500ms = (CVI_U32)(DEFAULT_FPS * 500 / 1000);
    stInitAttr.enGainMode       = SNS_GAIN_MODE_SHARE;

    if (g_pstSnsObj->pfnSetInit) {
        s32Ret = g_pstSnsObj->pfnSetInit(ViPipe, &stInitAttr);
        if (s32Ret != CVI_SUCCESS) {
            fprintf(stderr, "[ERROR] pfnSetInit: 0x%x\n", s32Ret);
            return -1;
        }
    }

    /* 注册 sensor 回调到 ISP/AE/AWB 系统 */
    if (g_pstSnsObj->pfnRegisterCallback) {
        s32Ret = g_pstSnsObj->pfnRegisterCallback(ViPipe, &stAeLib, &stAwbLib);
        if (s32Ret != CVI_SUCCESS) {
            fprintf(stderr, "[ERROR] pfnRegisterCallback: 0x%x\n", s32Ret);
            return -1;
        }
    }

    /* 注意: 不在此时 probe sensor，sensor 需要 MIPI clock/reset 后才响应 I2C */

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
    if (g_pstSnsObj->pfnGetRxAttr) {
        s32Ret = g_pstSnsObj->pfnGetRxAttr(VI_PIPE_NUM, &stRxAttr);
        if (s32Ret != CVI_SUCCESS) {
            fprintf(stderr, "[ERROR] pfnGetRxAttr: 0x%x\n", s32Ret);
            return -1;
        }
    }

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
    if (s32Ret != CVI_SUCCESS) {
        fprintf(stderr, "[ERROR] CVI_VI_SetDevNum: 0x%x\n", s32Ret);
        return -1;
    }

    s32Ret = CVI_VI_SetDevAttr(VI_DEV_NUM, &stDevAttr);
    if (s32Ret != CVI_SUCCESS) {
        fprintf(stderr, "[ERROR] CVI_VI_SetDevAttr: 0x%x\n", s32Ret);
        return -1;
    }

    s32Ret = CVI_VI_EnableDev(VI_DEV_NUM);
    if (s32Ret != CVI_SUCCESS) {
        fprintf(stderr, "[ERROR] CVI_VI_EnableDev: 0x%x\n", s32Ret);
        return -1;
    }

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
    if (g_pstSnsObj->pfnGetRxAttr) {
        s32Ret = g_pstSnsObj->pfnGetRxAttr(VI_PIPE_NUM, &stRxAttr);
        if (s32Ret != CVI_SUCCESS) {
            fprintf(stderr, "[ERROR] pfnGetRxAttr: 0x%x\n", s32Ret);
            return -1;
        }
    }

    s32Ret = CVI_MIPI_SetMipiAttr(VI_PIPE_NUM, (CVI_VOID *)&stRxAttr);
    if (s32Ret != CVI_SUCCESS) {
        fprintf(stderr, "[ERROR] CVI_MIPI_SetMipiAttr: 0x%x\n", s32Ret);
        return -1;
    }

    s32Ret = CVI_MIPI_SetSensorClock(0, 1);
    if (s32Ret != CVI_SUCCESS) {
        fprintf(stderr, "[ERROR] CVI_MIPI_SetSensorClock: 0x%x\n", s32Ret);
        return -1;
    }
    usleep(20);

    s32Ret = CVI_MIPI_SetSensorReset(0, 0);
    if (s32Ret != CVI_SUCCESS) {
        fprintf(stderr, "[ERROR] CVI_MIPI_SetSensorReset: 0x%x\n", s32Ret);
        return -1;
    }
    usleep(10000);

    printf("[MIPI] 初始化完成\n");
    return 0;
}

/* ================================================================
 * VI Pipe 创建
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
    if (s32Ret != CVI_SUCCESS) {
        fprintf(stderr, "[ERROR] CVI_VI_SetDevBindPipe: 0x%x\n", s32Ret);
        return -1;
    }

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
    if (s32Ret != CVI_SUCCESS) {
        fprintf(stderr, "[ERROR] CVI_VI_CreatePipe: 0x%x\n", s32Ret);
        return -1;
    }

    s32Ret = CVI_VI_StartPipe(VI_PIPE_NUM);
    if (s32Ret != CVI_SUCCESS) {
        fprintf(stderr, "[ERROR] CVI_VI_StartPipe: 0x%x\n", s32Ret);
        return -1;
    }

    printf("[VI] Pipe 创建完成\n");
    return 0;
}

/* ================================================================
 * VI Channel 创建 — cv181x 仍然需要 channel 作为 YUV 输出端口
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
    if (s32Ret != CVI_SUCCESS) {
        fprintf(stderr, "[ERROR] CVI_VI_SetChnAttr: 0x%x\n", s32Ret);
        return -1;
    }

    s32Ret = CVI_VI_EnableChn(VI_PIPE_NUM, VI_CHN_NUM);
    if (s32Ret != CVI_SUCCESS) {
        fprintf(stderr, "[ERROR] CVI_VI_EnableChn: 0x%x\n", s32Ret);
        return -1;
    }

    printf("[VI] Channel 创建完成\n");
    return 0;
}

/* ================================================================
 * ISP 线程 — CVI_ISP_Run 是阻塞调用，必须在独立线程中运行
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

    /* 1. 注册 AE library */
    stAeLib.s32Id = ViPipe;
    strncpy(stAeLib.acLibName, CVI_AE_LIB_NAME, sizeof(stAeLib.acLibName));
    s32Ret = CVI_AE_Register(ViPipe, &stAeLib);
    if (s32Ret != CVI_SUCCESS)
        fprintf(stderr, "[WARN] CVI_AE_Register: 0x%x\n", s32Ret);

    /* 2. 注册 AWB library */
    stAwbLib.s32Id = ViPipe;
    strncpy(stAwbLib.acLibName, CVI_AWB_LIB_NAME, sizeof(stAwbLib.acLibName));
    s32Ret = CVI_AWB_Register(ViPipe, &stAwbLib);
    if (s32Ret != CVI_SUCCESS)
        fprintf(stderr, "[WARN] CVI_AWB_Register: 0x%x\n", s32Ret);

    /* 3. 绑定 AE/AWB library 到 ISP */
    memset(&stBindAttr, 0, sizeof(stBindAttr));
    stBindAttr.sensorId          = 0;
    stBindAttr.stAeLib.s32Id     = ViPipe;
    strncpy(stBindAttr.stAeLib.acLibName, CVI_AE_LIB_NAME,
            sizeof(stBindAttr.stAeLib.acLibName));
    stBindAttr.stAwbLib.s32Id    = ViPipe;
    strncpy(stBindAttr.stAwbLib.acLibName, CVI_AWB_LIB_NAME,
            sizeof(stBindAttr.stAwbLib.acLibName));

    s32Ret = CVI_ISP_SetBindAttr(ViPipe, &stBindAttr);
    if (s32Ret != CVI_SUCCESS) {
        fprintf(stderr, "[ERROR] CVI_ISP_SetBindAttr: 0x%x\n", s32Ret);
        return -1;
    }

    /* 4. 分配 ISP 内存 */
    s32Ret = CVI_ISP_MemInit(ViPipe);
    if (s32Ret != CVI_SUCCESS) {
        fprintf(stderr, "[ERROR] CVI_ISP_MemInit: 0x%x\n", s32Ret);
        return -1;
    }

    /* 5. 设置公有属性 (必须在 Init 之前) */
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
    if (s32Ret != CVI_SUCCESS) {
        fprintf(stderr, "[ERROR] CVI_ISP_SetPubAttr: 0x%x\n", s32Ret);
        return -1;
    }

    /* 6. 初始化 ISP */
    s32Ret = CVI_ISP_Init(ViPipe);
    if (s32Ret != CVI_SUCCESS) {
        fprintf(stderr, "[ERROR] CVI_ISP_Init: 0x%x\n", s32Ret);
        return -1;
    }

    /* 7. 在线程中启动 ISP 处理 */
    if (pthread_create(&g_ispThread, NULL, isp_thread, &ViPipe) != 0) {
        fprintf(stderr, "[ERROR] pthread_create (ISP) 失败\n");
        return -1;
    }
    g_bIspRunning = 1;

    printf("[ISP] 初始化完成 (线程运行中)\n");
    return 0;
}

/* ================================================================
 * VPSS 初始化
 * ================================================================ */
static int vpss_init(VPSS_GRP VpssGrp, VPSS_CHN VpssChn)
{
    VPSS_GRP_ATTR_S stGrpAttr;
    VPSS_CHN_ATTR_S stChnAttr;
    CVI_S32 s32Ret;

    printf("[VPSS] 初始化 VPSS...\n");

    memset(&stGrpAttr, 0, sizeof(stGrpAttr));
    stGrpAttr.u32MaxW       = SENSOR_WIDTH;
    stGrpAttr.u32MaxH       = SENSOR_HEIGHT;
    stGrpAttr.enPixelFormat = VI_PIXEL_FORMAT;
    stGrpAttr.stFrameRate.s32SrcFrameRate = -1;
    stGrpAttr.stFrameRate.s32DstFrameRate = -1;
    stGrpAttr.u8VpssDev     = 0;

    s32Ret = CVI_VPSS_CreateGrp(VpssGrp, &stGrpAttr);
    if (s32Ret != CVI_SUCCESS) {
        fprintf(stderr, "[ERROR] CVI_VPSS_CreateGrp: 0x%x\n", s32Ret);
        return -1;
    }

    memset(&stChnAttr, 0, sizeof(stChnAttr));
    stChnAttr.u32Width       = OUTPUT_WIDTH;
    stChnAttr.u32Height      = OUTPUT_HEIGHT;
    stChnAttr.enVideoFormat  = VIDEO_FORMAT_LINEAR;
    stChnAttr.enPixelFormat  = PIXEL_FORMAT_NV12;
    stChnAttr.u32Depth       = 1;
    stChnAttr.stFrameRate.s32SrcFrameRate = -1;
    stChnAttr.stFrameRate.s32DstFrameRate = -1;

    s32Ret = CVI_VPSS_SetChnAttr(VpssGrp, VpssChn, &stChnAttr);
    if (s32Ret != CVI_SUCCESS) {
        fprintf(stderr, "[ERROR] CVI_VPSS_SetChnAttr: 0x%x\n", s32Ret);
        return -1;
    }

    s32Ret = CVI_VPSS_EnableChn(VpssGrp, VpssChn);
    if (s32Ret != CVI_SUCCESS) {
        fprintf(stderr, "[ERROR] CVI_VPSS_EnableChn: 0x%x\n", s32Ret);
        return -1;
    }

    s32Ret = CVI_VPSS_StartGrp(VpssGrp);
    if (s32Ret != CVI_SUCCESS) {
        fprintf(stderr, "[ERROR] CVI_VPSS_StartGrp: 0x%x\n", s32Ret);
        return -1;
    }

    printf("[VPSS] 初始化完成 (输出 %dx%d)\n", OUTPUT_WIDTH, OUTPUT_HEIGHT);
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
        fprintf(stderr, "[WARN] CVI_VB_SetConfig: 0x%x, 使用默认\n", s32Ret);

    s32Ret = CVI_VB_Init();
    if (s32Ret != CVI_SUCCESS) {
        fprintf(stderr, "[ERROR] CVI_VB_Init: 0x%x\n", s32Ret);
        return -1;
    }

    printf("[VB] 初始化完成\n");
    return 0;
}

/* ================================================================
 * 绑定 VI → VPSS
 * ================================================================ */
static int bind_vi_to_vpss(void)
{
    MMF_CHN_S stSrcChn, stDestChn;

    stSrcChn.enModId  = CVI_ID_VI;
    stSrcChn.s32DevId = VI_PIPE_NUM;
    stSrcChn.s32ChnId = 0;

    stDestChn.enModId  = CVI_ID_VPSS;
    stDestChn.s32DevId = VPSS_GRP_NUM;
    stDestChn.s32ChnId = 0;

    printf("[BIND] VI(Pipe%d) → VPSS(Grp%d)\n",
           (int)stSrcChn.s32DevId, (int)stDestChn.s32DevId);

    return CVI_SYS_Bind(&stSrcChn, &stDestChn);
}

/* ================================================================
 * 主采集循环
 * ================================================================ */
static int capture_loop(void)
{
    VIDEO_FRAME_INFO_S stVideoFrame;
    CVI_S32 s32Ret;

    if (g_acOutputDir[0] != '\0')
        mkdir(g_acOutputDir, 0755);

    printf("\n[CAPTURE] 开始采集 (Ctrl+C 停止)\n");
    printf("[CAPTURE] %dx%d @ %.1f fps\n",
           OUTPUT_WIDTH, OUTPUT_HEIGHT, g_fTargetFps);
    printf("===========================================\n\n");

    while (g_bRunning) {
        if (g_u32MaxFrames > 0 && g_u32FrameCount >= g_u32MaxFrames) {
            printf("[CAPTURE] 已采 %u 帧，退出\n", g_u32MaxFrames);
            break;
        }

        memset(&stVideoFrame, 0, sizeof(stVideoFrame));
        s32Ret = CVI_VPSS_GetChnFrame(VPSS_GRP_NUM, VPSS_CHN_NUM,
                                       &stVideoFrame, 1000);
        if (s32Ret != CVI_SUCCESS) {
            if (s32Ret == CVI_ERR_VPSS_BUF_EMPTY)
                continue;
            fprintf(stderr, "[ERROR] CVI_VPSS_GetChnFrame: 0x%x\n", s32Ret);
            break;
        }

        if (g_acOutputDir[0] != '\0')
            save_frame(&stVideoFrame, g_acOutputDir);

        CVI_VPSS_ReleaseChnFrame(VPSS_GRP_NUM, VPSS_CHN_NUM, &stVideoFrame);

        g_u32FrameCount++;

        if (g_u32FrameCount % 30 == 0)
            printf("[STATS] 帧 %u\n", g_u32FrameCount);
    }

    printf("\n[CAPTURE] 结束 (%u 帧)\n", g_u32FrameCount);
    return 0;
}

/* ================================================================
 * 清理
 * ================================================================ */
static void cleanup(void)
{
    MMF_CHN_S stSrcChn, stDestChn;

    printf("[CLEANUP] 释放资源...\n");

    stSrcChn.enModId  = CVI_ID_VI;
    stSrcChn.s32DevId = VI_PIPE_NUM;
    stSrcChn.s32ChnId = 0;
    stDestChn.enModId  = CVI_ID_VPSS;
    stDestChn.s32DevId = VPSS_GRP_NUM;
    stDestChn.s32ChnId = 0;
    CVI_SYS_UnBind(&stSrcChn, &stDestChn);

    CVI_VPSS_StopGrp(VPSS_GRP_NUM);
    CVI_VPSS_DisableChn(VPSS_GRP_NUM, VPSS_CHN_NUM);
    CVI_VPSS_DestroyGrp(VPSS_GRP_NUM);

    CVI_VI_DisableChn(VI_PIPE_NUM, VI_CHN_NUM);
    CVI_VI_StopPipe(VI_PIPE_NUM);
    CVI_VI_DestroyPipe(VI_PIPE_NUM);
    CVI_VI_DisableDev(VI_DEV_NUM);
    CVI_VI_SetDevNum(0);

    if (g_pstSnsObj && g_pstSnsObj->pfnUnRegisterCallback) {
        ALG_LIB_S stAeLib, stAwbLib;
        stAeLib.s32Id  = VI_PIPE_NUM;
        stAwbLib.s32Id = VI_PIPE_NUM;
        strncpy(stAeLib.acLibName, "ae", sizeof(stAeLib.acLibName));
        strncpy(stAwbLib.acLibName, "awb", sizeof(stAwbLib.acLibName));
        g_pstSnsObj->pfnUnRegisterCallback(VI_PIPE_NUM, &stAeLib, &stAwbLib);
    }

    CVI_ISP_Exit(VI_PIPE_NUM);

    /* 等待 ISP 线程退出 */
    if (g_bIspRunning) {
        pthread_join(g_ispThread, NULL);
        g_bIspRunning = 0;
    }

    CVI_VB_Exit();
    CVI_SYS_Exit();

    printf("[CLEANUP] 完成\n");
}

/* ================================================================
 * 参数解析
 * ================================================================ */
static void parse_args(int argc, char *argv[])
{
    int i;
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-o") && i + 1 < argc)
            strncpy(g_acOutputDir, argv[++i], sizeof(g_acOutputDir) - 1);
        else if (!strcmp(argv[i], "-n") && i + 1 < argc)
            g_u32MaxFrames = (unsigned int)atoi(argv[++i]);
        else if (!strcmp(argv[i], "-f") && i + 1 < argc)
            g_fTargetFps = atof(argv[++i]);
        else if (!strcmp(argv[i], "-h")) {
            printf("用法: %s [-o dir] [-n frames] [-f fps] [-h]\n", argv[0]);
            exit(0);
        }
    }
}

/* ================================================================
 * 主函数
 * ================================================================ */
int main(int argc, char *argv[])
{
    int ret;

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    parse_args(argc, argv);

    printf("===== Milk-V Duo GC2083 Camera =====\n");

    ret = CVI_SYS_Init();
    if (ret != CVI_SUCCESS) {
        fprintf(stderr, "[ERROR] CVI_SYS_Init: 0x%x\n", ret);
        return -1;
    }

    if (vb_init() != 0)  goto err;

    /* 关键：sensor 必须最先初始化，注册到 ISP/AE/AWB */
    if (sensor_init(VI_PIPE_NUM) != 0)  goto err;
    if (vi_dev_init() != 0)  goto err;
    if (mipi_init() != 0)   goto err;
    if (vi_pipe_init() != 0) goto err;
    if (isp_init(VI_PIPE_NUM) != 0) goto err;
    if (vi_chn_init() != 0)  goto err;
    if (vpss_init(VPSS_GRP_NUM, VPSS_CHN_NUM) != 0) goto err;

    ret = bind_vi_to_vpss();
    if (ret != CVI_SUCCESS) {
        fprintf(stderr, "[ERROR] CVI_SYS_Bind: 0x%x\n", ret);
        goto err;
    }

    capture_loop();

err:
    cleanup();
    return ret;
}
