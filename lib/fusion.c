/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <math.h>
#include <string.h>
#include "platform/helpers.h"

#include "fusion.h"

// Normalizes angle to -180 to 180 degrees
static f32 normalize_angle(f32 angle) {
    while (angle > 180.f) {
        angle -= 360.f;
    }
    while (angle < -180.f) {
        angle += 360.f;
    }
    return angle;
}

void fusion_init(FusionConfig *config, f32 sampleRate) {
    // Default complementary filter coefficient (98% gyro, 2% accel)
    // TODO: tune this?
    config->alpha = 0.98f;
    config->sampleRate = sampleRate;
    config->useGPS = false;
    // Initialize bias and offset arrays to zero
    memset(config->gyroBias, 0, sizeof(config->gyroBias));
    memset(config->accelOffset, 0, sizeof(config->accelOffset));
}

void fusion_reset(FusionState *state) {
    *state = (FusionState){0};
}

void fusion_update(FusionConfig *config, FusionState *state, const f32 accel[3], const f32 gyro[3], f32 gpsTrack,
                   f32 dt) {
    // Reject non-finite or negative delta times and fall back to configured sample period
    if (!isfinite(dt) || dt <= 0.0f) {
        dt = 1.0f / config->sampleRate;
    } else {
        // Prevent large one-off dt spikes from causing unrealistic attitude jumps
        dt = clampf(dt, 1.0f / (config->sampleRate * 2.0f), 0.1f);
    }

    // Apply calibration offsets
    f32 accel_cal[3] = {accel[0] - config->accelOffset[0], accel[1] - config->accelOffset[1],
                        accel[2] - config->accelOffset[2]};
    f32 gyro_cal[3] = {gyro[0] - config->gyroBias[0], gyro[1] - config->gyroBias[1], gyro[2] - config->gyroBias[2]};
    state->rollRate = gyro_cal[0];
    state->pitchRate = gyro_cal[1];
    state->yawRate = gyro_cal[2];

    // Calculate roll and pitch from accelerometer (using gravity vector)
    // Roll = rotation around X axis
    // Pitch = rotation around Y axis
    f32 accel_roll = degrees(atan2f(accel_cal[1], accel_cal[2]));
    f32 accel_pitch = degrees(atan2f(-accel_cal[0], sqrtf(accel_cal[1] * accel_cal[1] + accel_cal[2] * accel_cal[2])));

    // Initialize on first update
    if (!state->initialized) {
        state->roll = accel_roll;
        state->pitch = accel_pitch;
        state->yaw = isnan(gpsTrack) ? 0.0f : gpsTrack;
        state->initialized = true;
    }

    // Integrate gyroscope data
    f32 gyro_roll = state->roll + state->rollRate * dt;
    f32 gyro_pitch = state->pitch + state->pitchRate * dt;
    f32 gyro_yaw = state->yaw + state->yawRate * dt;
    // Complementary filter
    state->roll = config->alpha * gyro_roll + (1.0f - config->alpha) * accel_roll;
    state->pitch = config->alpha * gyro_pitch + (1.0f - config->alpha) * accel_pitch;

    // Yaw: primarily use gyro, maybe correct with GPS
    if (config->useGPS && !isnan(gpsTrack)) {
        f32 gps_error = normalize_angle(gpsTrack - gyro_yaw);
        state->yaw = normalize_angle(gyro_yaw + (1.0f - config->alpha) * gps_error);
    } else {
        state->yaw = normalize_angle(gyro_yaw); // No GPS
    }

    // Normalize all angles to -180 to 180 range
    state->roll = normalize_angle(state->roll);
    state->pitch = normalize_angle(state->pitch);
    state->updateCount++;
}

void fusion_calibration_start(FusionCalibration *cal) {
    memset(cal->gyroBiasSum, 0, sizeof(cal->gyroBiasSum));
    memset(cal->accelOffsetSum, 0, sizeof(cal->accelOffsetSum));
    cal->sampleCount = 0;
    cal->isCalibrating = true;
}

void fusion_calibration_add_sample(FusionCalibration *cal, const f32 accel[3], const f32 gyro[3]) {
    if (!cal->isCalibrating) {
        return;
    }

    // Accumulate gyro readings for bias calculation
    cal->gyroBiasSum[0] += gyro[0];
    cal->gyroBiasSum[1] += gyro[1];
    cal->gyroBiasSum[2] += gyro[2];
    // Accumulate accel readings for offset calculation
    // When level, we expect: X=0, Y=0, Z=-1g (gravity pointing down)
    cal->accelOffsetSum[0] += accel[0];
    cal->accelOffsetSum[1] += accel[1];
    cal->accelOffsetSum[2] += accel[2] + 1.0f; // +1g to compensate for gravity

    cal->sampleCount++;
}

bool fusion_calibration_finish(FusionCalibration *cal, FusionConfig *config) {
    if (!cal->isCalibrating || cal->sampleCount == 0) {
        return false;
    }

    // Calculate average bias and offsets
    f32 inv_count = 1.0f / (f32)cal->sampleCount;
    config->gyroBias[0] = cal->gyroBiasSum[0] * inv_count;
    config->gyroBias[1] = cal->gyroBiasSum[1] * inv_count;
    config->gyroBias[2] = cal->gyroBiasSum[2] * inv_count;
    config->accelOffset[0] = cal->accelOffsetSum[0] * inv_count;
    config->accelOffset[1] = cal->accelOffsetSum[1] * inv_count;
    config->accelOffset[2] = cal->accelOffsetSum[2] * inv_count;

    cal->isCalibrating = false;
    return true;
}

void fusion_load_calibration(FusionConfig *config, const f32 gyroBias[3], const f32 accelOffset[3]) {
    memcpy(config->gyroBias, gyroBias, sizeof(config->gyroBias));
    memcpy(config->accelOffset, accelOffset, sizeof(config->accelOffset));
}
