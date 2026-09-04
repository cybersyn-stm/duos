#ifndef STEP3_SENSOR_H
#define STEP3_SENSOR_H

#include <cvi_common.h>

/**
 * @brief Step3: Configure sensor parameters and probe hardware.
 *
 * Sets sensor operating parameters via I2C:
 *   - Gain mode:    SNS_GAIN_MODE_SHARE (exposure & gain share same setting)
 *   - MUX mode:     SNS_BDG_MUX_NONE (single sensor, no bridge chip)
 *   - I2C bus:      bus_id=2 (Duo board camera on I2C-2)
 *   - I2C address:  0x37 (GC2083 sensor address)
 *   - Image mode:   1920×1080 @30fps (via pfn_cmos_set_image_mode)
 *   - Hardware probe: reads chip ID register to verify GC2083 is on the bus
 *
 * @param vi_pipe  VI pipe number (0 for single camera).
 * @return CVI_SUCCESS on success, negative on error.
 */
CVI_S32 step3_sensor_init(VI_PIPE vi_pipe);

/**
 * @brief Step3b: Probe sensor hardware (reads chip ID over I2C).
 *
 * Must be called AFTER MIPI clock is enabled — GC2083 needs MIPI clock
 * for I2C to respond. Call this after step2_mipi_init().
 */
CVI_S32 step3b_sensor_probe(VI_PIPE vi_pipe);

#endif /* STEP3_SENSOR_H */
