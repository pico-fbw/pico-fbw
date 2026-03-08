/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <math.h>

#include "calibration.h"

#define FUSION_CALIBRATION_GYRO_STILL_THRESHOLD_DPS 3.0f // Max gyro rate while considered still
#define FUSION_CALIBRATION_ACCEL_MAG_MIN_G 0.80f         // Min accel magnitude while considered still
#define FUSION_CALIBRATION_ACCEL_MAG_MAX_G 1.20f         // Max accel magnitude while considered still
#define FUSION_CALIBRATION_LEVEL_MAIN_AXIS_MIN_G 0.75f   // One axis should dominate while level
#define FUSION_CALIBRATION_LEVEL_CROSS_AXIS_MAX_G 0.60f  // Remaining axes should stay small while level

bool fusion_attitude_calibration_level_and_still(const f32 accel[3], const f32 gyro[3]) {
    // Get the max gyro rate across all axes to check for stillness
    f32 maxGyro = fmaxf(fabsf(gyro[0]), fmaxf(fabsf(gyro[1]), fabsf(gyro[2])));
    if (maxGyro > FUSION_CALIBRATION_GYRO_STILL_THRESHOLD_DPS) {
        return false;
    }
    // Also check accel magnitude and distribution to check for levelness
    f32 accelMag = sqrtf(accel[0] * accel[0] + accel[1] * accel[1] + accel[2] * accel[2]);
    if (accelMag < FUSION_CALIBRATION_ACCEL_MAG_MIN_G || accelMag > FUSION_CALIBRATION_ACCEL_MAG_MAX_G) {
        return false;
    }

    f32 absX = fabsf(accel[0]);
    f32 absY = fabsf(accel[1]);
    f32 absZ = fabsf(accel[2]);
    f32 dominant = fmaxf(absX, fmaxf(absY, absZ));
    f32 cross = (absX + absY + absZ) - dominant;
    return dominant >= FUSION_CALIBRATION_LEVEL_MAIN_AXIS_MIN_G && cross <= FUSION_CALIBRATION_LEVEL_CROSS_AXIS_MAX_G;
}

void fusion_attitude_calibration_still_add_sample(FusionCalibrationContext *ctx, const f32 accel[3],
                                                  const f32 gyro[3]) {
    // Integrate this still sample into running accel/gyro sums
    ctx->stillAccelSum[0] += accel[0];
    ctx->stillAccelSum[1] += accel[1];
    ctx->stillAccelSum[2] += accel[2];
    ctx->stillGyroSum[0] += gyro[0];
    ctx->stillGyroSum[1] += gyro[1];
    ctx->stillGyroSum[2] += gyro[2];
    ctx->stillSampleCount++;
}

void fusion_attitude_calibration_still_finalize(FusionCalibrationContext *ctx) {
    if (ctx->stillSampleCount == 0) {
        return;
    }
    // Convert accumulated sums to per-axis means
    f32 invCount = 1.0f / (f32)ctx->stillSampleCount;
    for (u32 i = 0; i < 3; i++) {
        ctx->stillAccelAvg[i] = ctx->stillAccelSum[i] * invCount;
        ctx->stillGyroAvg[i] = ctx->stillGyroSum[i] * invCount;
    }
}
