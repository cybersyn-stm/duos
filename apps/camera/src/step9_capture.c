#include "step9_capture.h"

#include <cvi_sys.h>
#include <cvi_vpss.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* ── 零帧检测（抽样） ── */
static int is_frame_all_zero(const unsigned char *data, size_t len) {
    size_t step = 64;
    for (size_t i = 0; i < len; i += step) {
        if (data[i] != 0)
            return 0;
    }
    return 1;
}

/* ── 单帧捕获（保留兼容旧调用） ── */
CVI_S32 step9_capture_init(VPSS_GRP vpss_grp, VPSS_CHN vpss_chn) {
    CVI_S32 ret, mmap_size, width, height;
    void *mmp;
    VIDEO_FRAME_INFO_S stFrame;

    /* 丢弃 3 帧让管道稳定 */
    for (int i = 0; i < 3; i++) {
        memset(&stFrame, 0, sizeof(stFrame));
        ret = CVI_VPSS_GetChnFrame(vpss_grp, vpss_chn, &stFrame, 1000);
        if (ret == CVI_SUCCESS)
            CVI_VPSS_ReleaseChnFrame(vpss_grp, vpss_chn, &stFrame);
    }

    for (int attempt = 0; attempt < 5; attempt++) {
        memset(&stFrame, 0, sizeof(stFrame));
        ret = CVI_VPSS_GetChnFrame(vpss_grp, vpss_chn, &stFrame, 2000);
        if (ret != CVI_SUCCESS) {
            printf("[CAPTURE] GetChnFrame fail: %#x (attempt %d)\n", ret,
                   attempt + 1);
            continue;
        }

        width = stFrame.stVFrame.u32Width;
        height = stFrame.stVFrame.u32Height;
        if (width == 0 || height == 0) {
            printf("[CAPTURE] Invalid resolution %ux%u (attempt %d)\n", width,
                   height, attempt + 1);
            CVI_VPSS_ReleaseChnFrame(vpss_grp, vpss_chn, &stFrame);
            continue;
        }

        mmap_size = stFrame.stVFrame.u32Stride[0] * height * 3 / 2;
        if (mmap_size <= 0) {
            printf("[CAPTURE] Invalid mmap_size %d (attempt %d)\n", mmap_size,
                   attempt + 1);
            CVI_VPSS_ReleaseChnFrame(vpss_grp, vpss_chn, &stFrame);
            continue;
        }

        mmp = CVI_SYS_Mmap(stFrame.stVFrame.u64PhyAddr[0], mmap_size);
        if (mmp == NULL) {
            printf("[CAPTURE] Mmap failed (attempt %d)\n", attempt + 1);
            CVI_VPSS_ReleaseChnFrame(vpss_grp, vpss_chn, &stFrame);
            continue;
        }

        if (is_frame_all_zero((const unsigned char *)mmp, (size_t)mmap_size)) {
            printf("[CAPTURE] Zero frame, retrying (%d/5)...\n", attempt + 1);
            CVI_SYS_Munmap(mmp, mmap_size);
            CVI_VPSS_ReleaseChnFrame(vpss_grp, vpss_chn, &stFrame);
            usleep(200000);
            continue;
        }

        FILE *fp = fopen("capture.yuv", "wb");
        if (fp) {
            fwrite(mmp, 1, (size_t)mmap_size, fp);
            fclose(fp);
            printf("[CAPTURE] Saved %ux%u (%d bytes)\n", width, height,
                   mmap_size);
        }
        CVI_SYS_Munmap(mmp, mmap_size);
        CVI_VPSS_ReleaseChnFrame(vpss_grp, vpss_chn, &stFrame);
        return (fp != NULL) ? CVI_SUCCESS : -1;
    }
    printf("[CAPTURE] All retries exhausted\n");
    return -1;
}

/* ── 批量连拍：base_001.yuv … base_NNN.yuv，全速 ── */
/* 返回实际保存的帧数；run 为 0 时立即中止                 */
CVI_S32 step9_capture_sequence(VPSS_GRP vpss_grp, VPSS_CHN vpss_chn,
                                const char *base, int count,
                                volatile int *run)
{
    VIDEO_FRAME_INFO_S stFrame;
    void *mmp;
    CVI_S32 ret, mmap_size, width, height;
    int saved = 0, zero_skipped = 0, consecutive_fail = 0;
    char fname[256];
    FILE *fp;

    printf("[SEQ] %s → %d frames\n", base, count);

    /* 预热：丢弃前 3 帧 */
    for (int i = 0; i < 3 && *run; i++) {
        memset(&stFrame, 0, sizeof(stFrame));
        ret = CVI_VPSS_GetChnFrame(vpss_grp, vpss_chn, &stFrame, 100);
        if (ret == CVI_SUCCESS)
            CVI_VPSS_ReleaseChnFrame(vpss_grp, vpss_chn, &stFrame);
        else
            usleep(10000);
    }

    /* 主循环 */
    while (saved < count && *run && consecutive_fail < 60) {
        memset(&stFrame, 0, sizeof(stFrame));
        ret = CVI_VPSS_GetChnFrame(vpss_grp, vpss_chn, &stFrame, 200);
        if (ret != CVI_SUCCESS) {
            consecutive_fail++;
            usleep(10000);  /* 让出 CPU，等新帧到达 */
            continue;
        }

        consecutive_fail = 0;

        width  = stFrame.stVFrame.u32Width;
        height = stFrame.stVFrame.u32Height;
        if (width == 0 || height == 0) {
            CVI_VPSS_ReleaseChnFrame(vpss_grp, vpss_chn, &stFrame);
            continue;
        }

        mmap_size = stFrame.stVFrame.u32Stride[0] * height * 3 / 2;
        if (mmap_size <= 0) {
            CVI_VPSS_ReleaseChnFrame(vpss_grp, vpss_chn, &stFrame);
            continue;
        }

        mmp = CVI_SYS_Mmap(stFrame.stVFrame.u64PhyAddr[0], mmap_size);
        if (mmp == NULL) {
            CVI_VPSS_ReleaseChnFrame(vpss_grp, vpss_chn, &stFrame);
            continue;
        }

        if (is_frame_all_zero((const unsigned char *)mmp, (size_t)mmap_size)) {
            CVI_SYS_Munmap(mmp, mmap_size);
            CVI_VPSS_ReleaseChnFrame(vpss_grp, vpss_chn, &stFrame);
            zero_skipped++;
            continue;
        }

        snprintf(fname, sizeof(fname), "%s_%03d.yuv", base, saved + 1);
        fp = fopen(fname, "wb");
        if (fp) {
            fwrite(mmp, 1, (size_t)mmap_size, fp);
            fclose(fp);
            saved++;
        }

        CVI_SYS_Munmap(mmp, mmap_size);
        CVI_VPSS_ReleaseChnFrame(vpss_grp, vpss_chn, &stFrame);

        /* 让出 CPU 给 streaming 线程，防止 VPSS 缓冲区耗尽 */
        usleep(40000);

        /* 每 10 张打印一次进度 */
        if (saved % 10 == 0)
            printf("  %d/%d\n", saved, count);
    }

    if (!*run)
        printf("[SEQ] Aborted\n");
    else if (consecutive_fail >= 60)
        printf("[SEQ] Timeout (>30s)\n");

    printf("[SEQ] %d/%d saved", saved, count);
    if (zero_skipped > 0)
        printf(", %d zero skipped", zero_skipped);
    printf("\n");
    return saved;
}
