#ifndef STEP4_VI_H
#define STEP4_VI_H

#include <cvi_common.h>

/**
 * @brief Step4: Configure and enable VI (Video Input) device.
 *
 * Sets up the MIPI RX input path from the sensor into the VI hardware block.
 *
 * Parameters (from sensor datasheet / SDK convention):
 *   - IntfMode:    VI_MODE_MIPI (GC2083 uses MIPI interface)
 *   - WorkMode:    VI_WORK_MODE_1Multiplex (single sensor, one input)
 *   - Resolution:  1920×1080 (must match Step3 image mode)
 *   - BayerFormat: BAYER_FORMAT_RG (GC2083 CFA pattern, from datasheet)
 *   - InputType:   VI_DATA_TYPE_RGB (raw Bayer classified as RGB)
 *   - WDR:         WDR_MODE_NONE (GC2083 doesn't support wide dynamic range)
 *   - CacheLine:   1080 (non-WDR mode, set to frame height)
 *   - snrFps:      30 (must match Step3 frame rate)
 *
 * Flow: SetDevAttr → EnableDev → SetDevBindPipe (register pipe ID only;
 * actual pipe created in Step5 via CreatePipe).
 *
 * @param vi_dev  VI device number (0 for single camera).
 * @return CVI_SUCCESS on success, negative on error.
 */
CVI_S32 step4_VI_init(VI_DEV vi_dev);

#endif /* STEP4_VI_H */
