/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/time.h"

#include "sys/configuration.h"

#include "control.h"

static f32 lastRollUpdate = 0, lastPitchUpdate = 0;

static f32 get_roll_dps(f32 roll) {
    return mapf(roll, -90.f, 90.f, -config.control[CONTROL_MAX_ROLL_RATE], config.control[CONTROL_MAX_ROLL_RATE]);
}

static f32 get_pitch_dps(f32 pitch) {
    return mapf(pitch, -90.f, 90.f, -config.control[CONTROL_MAX_PITCH_RATE], config.control[CONTROL_MAX_PITCH_RATE]);
}

static f32 calc_roll_adjust(f32 roll) {
    if (lastRollUpdate == 0)
        lastRollUpdate = time_s();          // Initialize lastUpdate to current time
    f32 deltaT = time_s() - lastRollUpdate; // Time since last call to this function
    lastRollUpdate = time_s();
    return get_roll_dps(roll) * deltaT;
}

static f32 calc_pitch_adjust(f32 pitch) {
    if (lastPitchUpdate == 0)
        lastPitchUpdate = time_s();
    f32 deltaT = time_s() - lastPitchUpdate;
    lastPitchUpdate = time_s();
    return get_pitch_dps(pitch) * deltaT;
}

f32 control_get_dps(Axis axis, f32 roll, f32 pitch) {
    switch (axis) {
        case ROLL:
            return get_roll_dps(roll);
        case PITCH:
            return get_pitch_dps(pitch);
        default:
            return 0.f;
    }
}

f32 control_calc_adjust(Axis axis, f32 roll, f32 pitch) {
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

f32 control_mix_elevon(Elevon elevon, f64 roll, f64 pitch) {
    f32 rollComponent = ((bool)config.pins[PINS_REVERSE_ROLL] ? -1 : 1) * roll * config.control[CONTROL_AIL_MIXING_BIAS];
    f32 pitchComponent = ((bool)config.pins[PINS_REVERSE_PITCH] ? -1 : 1) * pitch * config.control[CONTROL_ELEV_MIXING_BIAS];
    if (elevon == LEFT) {
        return (rollComponent + pitchComponent) * config.control[CONTROL_ELEVON_MIXING_GAIN] + 90.f;
    } else
        return (rollComponent - pitchComponent) * config.control[CONTROL_ELEVON_MIXING_GAIN] + 90.f;
}
