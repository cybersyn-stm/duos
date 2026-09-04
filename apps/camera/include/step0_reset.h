#ifndef STEP0_RESET_H
#define STEP0_RESET_H

#include "cvi_type.h"
#include "cvi_vi.h"
#include "cvi_vpss.h"

void step0_force_cleanup(void);
void step0_reset(VI_PIPE vi_pipe, VPSS_GRP vpss_grp);

#endif
