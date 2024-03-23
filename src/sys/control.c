/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/int.h"
#include "platform/time.h"

#include "sys/configuration.h"

#include "control.h"

static float lastRollUpdate = 0, lastPitchUpdate = 0;

static float get_roll_dps(float roll) {
    return mapf(roll, -90.f, 90.f, -config.control[CONTROL_MAX_ROLL_RATE], config.control[CONTROL_MAX_ROLL_RATE]);
}

static float get_pitch_dps(float pitch) {
    return mapf(pitch, -90.f, 90.f, -config.control[CONTROL_MAX_PITCH_RATE], config.control[CONTROL_MAX_PITCH_RATE]);
}

static float calc_roll_adjust(float roll) {
    if (lastRollUpdate == 0)
        lastRollUpdate = time_s();            // Initialize lastUpdate to current time
    float deltaT = time_s() - lastRollUpdate; // Time since last call to this function
    lastRollUpdate = time_s();
    return get_roll_dps(roll) * deltaT;
}

static float calc_pitch_adjust(float pitch) {
    if (lastPitchUpdate == 0)
        lastPitchUpdate = time_s();
    float deltaT = time_s() - lastPitchUpdate;
    lastPitchUpdate = time_s();
    return get_pitch_dps(pitch) * deltaT;
}

float control_get_dps(Axis axis, float roll, float pitch) {
    switch (axis) {
        case ROLL:
            return get_roll_dps(roll);
        case PITCH:
            return get_pitch_dps(pitch);
        default:
            return 0.f;
    }
}

float control_calc_adjust(Axis axis, float roll, float pitch) {
    switch (axis) {
        case ROLL:
            return calc_roll_adjust(roll);
        case PITCH:
            return calc_pitch_adjust(pitch);
        default:
            return 0.f;
    }
}

void control_reset() {
    lastRollUpdate = 0;
    lastPitchUpdate = 0;
}

float control_mix_elevon(Elevon elevon, double roll, double pitch) {
    float rollComponent = ((bool)config.pins[PINS_REVERSE_ROLL] ? -1 : 1) * roll * config.control[CONTROL_AIL_MIXING_BIAS];
    float pitchComponent = ((bool)config.pins[PINS_REVERSE_PITCH] ? -1 : 1) * pitch * config.control[CONTROL_ELEV_MIXING_BIAS];
    if (elevon == LEFT) {
        return (rollComponent + pitchComponent) * config.control[CONTROL_ELEVON_MIXING_GAIN] + 90.f;
    } else
        return (rollComponent - pitchComponent) * config.control[CONTROL_ELEVON_MIXING_GAIN] + 90.f;
}
