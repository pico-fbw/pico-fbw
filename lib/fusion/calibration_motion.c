/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <math.h>
#include <string.h>

#include "calibration.h"

#define FUSION_CALIBRATION_MOTION_THRESHOLD_DPS 12.0f   // Motion capture threshold
#define FUSION_CALIBRATION_MOTION_SIGN_LOCK_DPS 25.0f   // First strong sample locks direction
#define FUSION_CALIBRATION_MOTION_MIN_PEAK_DPS 20.f     // Minimum peak rate to accept motion
#define FUSION_CALIBRATION_MOTION_DOMINANCE_RATIO 1.20f // Dominant axis vs secondary axis ratio
#define FUSION_CALIBRATION_MOTION_SIGN_SUM_MIN 25.0f    // Fallback sign confidence threshold

void fusion_attitude_calibration_motion_reset(FusionCalibrationContext *ctx) {
    memset(ctx->motionPeakAbs, 0, sizeof(ctx->motionPeakAbs));
    memset(ctx->motionAbsSum, 0, sizeof(ctx->motionAbsSum));
    memset(ctx->motionSignedSum, 0, sizeof(ctx->motionSignedSum));
    memset(ctx->motionInitialSign, 0, sizeof(ctx->motionInitialSign));
    ctx->motionDetected = false;
    ctx->lastMotion = timestamp_now();
}

bool fusion_attitude_calibration_motion_active(const f32 centeredGyro[3]) {
    return fabsf(centeredGyro[0]) > FUSION_CALIBRATION_MOTION_THRESHOLD_DPS ||
           fabsf(centeredGyro[1]) > FUSION_CALIBRATION_MOTION_THRESHOLD_DPS ||
           fabsf(centeredGyro[2]) > FUSION_CALIBRATION_MOTION_THRESHOLD_DPS;
}

void fusion_attitude_calibration_motion_accumulate(FusionCalibrationContext *ctx, const f32 centeredGyro[3]) {
    for (u32 i = 0; i < 3; i++) {
        f32 rate = centeredGyro[i];
        f32 absRate = fabsf(rate);
        // Track strongest instantaneous rate seen for each axis
        if (absRate > ctx->motionPeakAbs[i]) {
            ctx->motionPeakAbs[i] = absRate;
        }
        // Track integrated absolute and signed activity across the maneuver
        ctx->motionAbsSum[i] += absRate;
        ctx->motionSignedSum[i] += rate;
        if (ctx->motionInitialSign[i] == 0 && absRate >= FUSION_CALIBRATION_MOTION_SIGN_LOCK_DPS) {
            // First sufficiently strong directional sample "locks" expected sign
            ctx->motionInitialSign[i] = rate >= 0.f ? 1 : -1;
        }
    }
}

bool fusion_attitude_calibration_motion_select_axis(const FusionCalibrationContext *ctx, i8 *sensorAxis,
                                                    i8 *sensorSign) {
    f32 best = -1.0f;
    f32 second = -1.0f;
    u32 bestAxis = 0;

    for (u32 i = 0; i < 3; i++) {
        f32 strength = ctx->motionAbsSum[i];
        // Pick dominant and second-dominant axes by integrated absolute motion
        if (strength > best) {
            second = best;
            best = strength;
            bestAxis = i;
        } else if (strength > second) {
            second = strength;
        }
    }

    if (ctx->motionPeakAbs[bestAxis] < FUSION_CALIBRATION_MOTION_MIN_PEAK_DPS) {
        return false;
    }
    if (second > 0.f && best < second * FUSION_CALIBRATION_MOTION_DOMINANCE_RATIO) {
        // Reject ambiguous captures where dominant axis is not clearly stronger
        return false;
    }
    if (ctx->rawAxisUsed[bestAxis]) {
        // Reject reusing a sensor axis already claimed by another body axis
        return false;
    }

    i8 sign = ctx->motionInitialSign[bestAxis];
    if (sign == 0) {
        // If no strong sign lock happened, fall back to integrated signed sum
        f32 fallback = ctx->motionSignedSum[bestAxis];
        if (fabsf(fallback) < FUSION_CALIBRATION_MOTION_SIGN_SUM_MIN) {
            return false;
        }
        sign = fallback >= 0.f ? 1 : -1;
    }

    *sensorAxis = (i8)bestAxis;
    *sensorSign = sign;
    return true;
}
