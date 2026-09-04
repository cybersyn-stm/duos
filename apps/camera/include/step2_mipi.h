#ifndef STEP2_MIPI_H
#define STEP2_MIPI_H

#include <cvi_common.h>

/**
 * @brief Step2: Initialize MIPI RX and enable sensor clock.
 *
 * Flow:
 *   1. pfnRegisterCallback → activates sensor library internal context
 *      (required before reading MIPI attributes; AE/AWB structs here
 *       are placeholders — real algorithm binding happens in Step6)
 *   2. pfnGetRxAttr → reads MIPI RX config from sensor object
 *      (lane count, clock frequency, etc.)
 *   3. Hardware reset sequence:
 *        Reset sensor → Reset MIPI → Set MIPI attributes → Clock on →
 *        Unreset sensor
 *
 * @param vi_pipe  VI pipe number (0 for single camera).
 * @return CVI_SUCCESS on success, negative on error.
 */
CVI_S32 step2_mipi_init(VI_PIPE vi_pipe);

#endif /* STEP2_MIPI_H */
