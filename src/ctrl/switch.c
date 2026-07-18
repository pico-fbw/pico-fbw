/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include "platform/types.h"

#include "ctrl/aircraft.h"
#include "io/receiver.h"
#include "sys/configuration.h"
#include "sys/flightplan.h"

#include "switch.h"

typedef enum SwitchPosition {
    SWITCH_POSITION_LOW,
    SWITCH_POSITION_MID,
    SWITCH_POSITION_HIGH,
} SwitchPosition;

#define SWITCH_DEBOUNCE_SAMPLES 3 // Number of consecutive matching readings required before committing a mode change
#define SWITCH_HYSTERESIS_DEG 8.f // Hysteresis margin (in degrees) around switch thresholds

static SwitchPosition lastPos = SWITCH_POSITION_LOW; // Aircraft begins in direct mode (aka low position)
static SwitchPosition pendingPos = SWITCH_POSITION_LOW;
static u8 pendingCount = 0;

/**
 * @param deg the current angle of the switch input in degrees (0-180)
 * @param last_pos the last stable position of the switch, used for hysteresis
 * @return position of the mode switch based on the current switch type and "angle"
 */
static SwitchPosition deg_to_pos(f32 deg, SwitchPosition last_pos) {
    switch ((SwitchType)config.general.switchType) {
        case SWITCH_TYPE_2_POS:
            // Hysteresis: boundary changes based on the last position to prevent jitter
            // The switch must be moved past the expanded boundary to trigger a mode change
            if (last_pos == SWITCH_POSITION_LOW) {
                return deg > 90.f + SWITCH_HYSTERESIS_DEG ? SWITCH_POSITION_HIGH : SWITCH_POSITION_LOW;
            }
            return deg < 90.f - SWITCH_HYSTERESIS_DEG ? SWITCH_POSITION_LOW : SWITCH_POSITION_HIGH;
        case SWITCH_TYPE_3_POS:
            switch (last_pos) {
                case SWITCH_POSITION_LOW:
                    return deg > 45.f + SWITCH_HYSTERESIS_DEG ? SWITCH_POSITION_MID : SWITCH_POSITION_LOW;
                case SWITCH_POSITION_HIGH:
                    return deg < 135.f - SWITCH_HYSTERESIS_DEG ? SWITCH_POSITION_MID : SWITCH_POSITION_HIGH;
                case SWITCH_POSITION_MID:
                    if (deg < 45.f - SWITCH_HYSTERESIS_DEG) {
                        return SWITCH_POSITION_LOW;
                    }
                    if (deg > 135.f + SWITCH_HYSTERESIS_DEG) {
                        return SWITCH_POSITION_HIGH;
                    }
                    return SWITCH_POSITION_MID;
            }
    }
    return SWITCH_POSITION_LOW; // Default to low position if some error occurs (should never happen)
}

void switch_update() {
    f32 switchDeg = receiver_get((i16)config.pins.inputSwitch, RECEIVER_MODE_DEGREE);
    SwitchPosition pos = deg_to_pos(switchDeg, lastPos);
    // The mode will only be changed when the user moves the switch;
    // the system's mode changes can persist and won't instantly be overrided by the switch
    if (lastPos == pos) {
        pendingPos = lastPos;
        pendingCount = 0;
        return;
    }

    // A change has been observed, but we will wait to see if it persists to avoid reacting to noise/jitter on the
    // switch input
    if (pendingPos != pos) {
        pendingPos = pos;
        pendingCount = 1;
        return;
    }
    pendingCount++;
    if (pendingCount < SWITCH_DEBOUNCE_SAMPLES) {
        return;
    }

    // This is a confirmed switch flip, execute the mode change
    switch (pos) {
        case SWITCH_POSITION_LOW:
            aircraft_change_mode(MODE_DIRECT);
            break;
        case SWITCH_POSITION_MID:
            aircraft_change_mode(MODE_NORMAL);
            break;
        case SWITCH_POSITION_HIGH:
            switch ((SwitchType)config.general.switchType) {
                case SWITCH_TYPE_2_POS:
                    // For 2-position switches, auto-select auto or normal mode based on if a flight plan is present
                    if (flightplan_get_active()) {
                        aircraft_change_mode(MODE_AUTO);
                    } else {
                        aircraft_change_mode(MODE_NORMAL);
                    }
                    break;
                case SWITCH_TYPE_3_POS:
                    aircraft_change_mode(MODE_AUTO);
                    break;
            }
            break;
    }
    lastPos = pos;
    pendingPos = pos;
    pendingCount = 0;
}
