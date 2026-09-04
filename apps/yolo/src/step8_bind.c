#include "step8_bind.h"
#include "cvi_common.h"
#include "cvi_sys.h"

#include <cvi_vi.h>
#include <stdio.h>
#include <string.h>

CVI_S32 step8_bind_init(VI_PIPE vi_pipe, VPSS_GRP vpss_grp) {
    CVI_S32 ret;
    MMF_CHN_S src_mmf_chn;
    memset(&src_mmf_chn, 0, sizeof(src_mmf_chn));
    src_mmf_chn.enModId = CVI_ID_VI;
    src_mmf_chn.s32DevId = vi_pipe;
    src_mmf_chn.s32ChnId = 0;

    MMF_CHN_S dest_mmf_chn;
    memset(&dest_mmf_chn, 0, sizeof(dest_mmf_chn));
    dest_mmf_chn.enModId = CVI_ID_VPSS;
    dest_mmf_chn.s32DevId = vpss_grp;
    dest_mmf_chn.s32ChnId = 0;

    ret = CVI_SYS_Bind(&src_mmf_chn, &dest_mmf_chn);
    if (ret != CVI_SUCCESS) { printf("[BIND] Fail: %#x\n", ret); return ret; }
    printf("[BIND] OK\n");

    return CVI_SUCCESS;
}
