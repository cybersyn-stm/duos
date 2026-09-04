#include "step0_reset.h"

#include "cvi_common.h"
#include "cvi_vi.h"
#include "cvi_vpss.h"
#include "cvi_sys.h"
#include "cvi_vb.h"
#include "cvi_venc.h"

#include <stdio.h>

/*
 * Best-effort cleanup of any leftover state from a previous unclean exit (kill -9).
 * All errors are silently ignored — resources may or may not exist.
 */
void step0_force_cleanup(void)
{
    /* Briefly init SYS so we can call cleanup APIs */
    CVI_SYS_Init();

    /* ── VENC consumer first ── */
    CVI_VENC_StopRecvFrame(0);
    CVI_VENC_DestroyChn(0);

    /* Unbind VPSS CHN1 → VENC */
    {
        MMF_CHN_S src = { .enModId = CVI_ID_VPSS, .s32DevId = 0, .s32ChnId = 1 };
        MMF_CHN_S dst = { .enModId = CVI_ID_VENC, .s32DevId = 0, .s32ChnId = 0 };
        CVI_SYS_UnBind(&src, &dst);
    }

    /* Unbind VI → VPSS CHN0 */
    {
        MMF_CHN_S src = { .enModId = CVI_ID_VI,   .s32DevId = 0, .s32ChnId = 0 };
        MMF_CHN_S dst = { .enModId = CVI_ID_VPSS, .s32DevId = 0, .s32ChnId = 0 };
        CVI_SYS_UnBind(&src, &dst);
    }

    /* ── VPSS ── */
    CVI_VPSS_StopGrp(0);
    CVI_VPSS_DisableChn(0, VPSS_CHN0);
    CVI_VPSS_DisableChn(0, VPSS_CHN1);
    CVI_VPSS_DisableChn(0, VPSS_CHN2);
    CVI_VPSS_DestroyGrp(0);

    /* ── VI ── */
    CVI_VI_DisableChn(0, 0);
    CVI_VI_StopPipe(0);
    CVI_VI_DestroyPipe(0);
    CVI_VI_DisableDev(0);

    CVI_VB_Exit();
    CVI_SYS_Exit();
    printf("[CLEANUP] Stale state cleared (if any)\n");
}

void step0_reset(VI_PIPE vi_pipe, VPSS_GRP vpss_grp)
{
    printf("\n=== Reset ===\n");

    /* Stop consumers (VPSS) first */
    CVI_VPSS_StopGrp(vpss_grp);
    CVI_VPSS_DisableChn(vpss_grp, VPSS_CHN0);
    CVI_VPSS_DisableChn(vpss_grp, VPSS_CHN1);
    CVI_VPSS_DisableChn(vpss_grp, VPSS_CHN2);

    /* Stop producer (VI Chn) */
    CVI_VI_DisableChn(vi_pipe, 0);

    /* Unbind */
    MMF_CHN_S src = { .enModId = CVI_ID_VI,  .s32DevId = vi_pipe,  .s32ChnId = 0 };
    MMF_CHN_S dst = { .enModId = CVI_ID_VPSS, .s32DevId = vpss_grp, .s32ChnId = 0 };
    CVI_SYS_UnBind(&src, &dst);

    /* Destroy */
    CVI_VPSS_DestroyGrp(vpss_grp);
    CVI_VI_StopPipe(vi_pipe);
    CVI_VI_DestroyPipe(vi_pipe);
    CVI_VI_DisableDev(vi_pipe);

    CVI_VB_Exit();
    CVI_SYS_Exit();

    printf("=== Reset: DONE ===\n\n");
}
