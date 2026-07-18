/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <stdio.h>
#include <string.h>
#include "platform/helpers.h"
#include "platform/wifi.h"

#include "ctrl/switch.h"
#include "io/gps.h"
#include "io/receiver.h"

#include "configuration.h"

typedef struct {
    i16 value;
    bool enabled;
} PinCheck;

/**
 * @return true if `value` is within `min` and `max`
 */
static bool validate_number_range(f32 value, f32 min, f32 max) {
    return value >= min && value <= max;
}

/**
 * Validates whether all pins are unique, taking into account the current control mode.
 * @param error a buffer to store a possible error message in
 * @param error_size the size of the error buffer
 * @return true if all pins are unique
 * @note The buffer should be at least 128 bytes long
 */
static bool validate_pin_uniqueness(char *error, size_t error_size) {
    const ControlMode mode = (ControlMode)config.general.controlMode;
    const bool usesRudder = mode == CTRLMODE_3AXIS || mode == CTRLMODE_3AXIS_ATHR;
    const bool usesThrottle =
        mode == CTRLMODE_3AXIS_ATHR || mode == CTRLMODE_2AXIS_ATHR || mode == CTRLMODE_FLYINGWING_ATHR;
    // Skip checking pins if they are not utilized in our current control mode
    const PinCheck pins[] = {
        {(i16)config.pins.inputAil, true},
        {(i16)config.pins.servoAil, true},
        {(i16)config.pins.inputEle, true},
        {(i16)config.pins.servoEle, true},
        {(i16)config.pins.inputRud, usesRudder},
        {(i16)config.pins.servoRud, true},
        {(i16)config.pins.inputThrottle, usesThrottle},
        {(i16)config.pins.escThrottle, usesThrottle},
        {(i16)config.pins.inputSwitch, true},
        {(i16)config.pins.servoBay, true},
        {(i16)config.pins.i2cSda, true},
        {(i16)config.pins.i2cScl, true},
        {(i16)config.pins.gpsTx, true},
        {(i16)config.pins.gpsRx, true},
    };

    for (size_t i = 0; i < count_of(pins); ++i) {
        if (!pins[i].enabled || pins[i].value < 0) {
            continue;
        }

        for (size_t j = i + 1; j < count_of(pins); ++j) {
            if (!pins[j].enabled || pins[j].value < 0) {
                continue;
            }

            if (pins[i].value == pins[j].value) {
                snprintf(error, error_size, "A pin may only be used once.");
                return false;
            }
        }
    }
    return true;
}

bool config_validate(char *error, size_t error_size) {
    if ((ControlMode)config.general.controlMode < CTRLMODE_MIN ||
        (ControlMode)config.general.controlMode > CTRLMODE_MAX) {
        snprintf(error, error_size, "Control mode must be between %d and %d.", CTRLMODE_MIN, CTRLMODE_MAX);
        return false;
    }
    if ((SwitchType)config.general.switchType < SWITCH_TYPE_MIN ||
        (SwitchType)config.general.switchType > SWITCH_TYPE_MAX) {
        snprintf(error, error_size, "Switch type must be between %d and %d.", SWITCH_TYPE_MIN, SWITCH_TYPE_MAX);
        return false;
    }
    if ((WifiEnabled)config.general.wifiEnabled < WIFI_ENABLED_MIN ||
        (WifiEnabled)config.general.wifiEnabled > WIFI_ENABLED_MAX) {
        snprintf(error, error_size, "Wi-Fi enable status must be between %d and %d.", WIFI_ENABLED_MIN,
                 WIFI_ENABLED_MAX);
        return false;
    }
    if ((GPSCommandType)config.sensors.gpsCommandType < GPS_COMMAND_TYPE_MIN ||
        (GPSCommandType)config.sensors.gpsCommandType > GPS_COMMAND_TYPE_MAX) {
        snprintf(error, error_size, "GPS command type must be between %d and %d.", GPS_COMMAND_TYPE_MIN,
                 GPS_COMMAND_TYPE_MAX);
        return false;
    }
    if (!validate_pin_uniqueness(error, error_size)) {
        return false;
    }
    if (!validate_number_range(config.control.expo, 0.f, 1.f)) {
        snprintf(error, error_size, "Expo must be between 0 and 1.");
        return false;
    }
    if (!validate_number_range(config.control.rollLimit, 0.f, 72.f)) {
        snprintf(error, error_size, "Roll limit must be between 0 and 72 degrees.");
        return false;
    }
    if (!validate_number_range(config.control.rollLimitHold, 0.f, 72.f)) {
        snprintf(error, error_size, "Roll limit hold must be between 0 and 72 degrees.");
        return false;
    }
    if (!validate_number_range(config.control.pitchUpperLimit, 0.f, 35.f)) {
        snprintf(error, error_size, "Upper pitch limit must be between 0 and 35 degrees.");
        return false;
    }
    if (!validate_number_range(config.control.pitchLowerLimit, -20.f, 0.f)) {
        snprintf(error, error_size, "Lower pitch limit must be between -20 and 0 degrees.");
        return false;
    }
    if (!validate_number_range(config.control.throttleSensitivity, 0.f, 1.f)) {
        snprintf(error, error_size, "Throttle sensitivity must be between 0.0 and 1.0.");
        return false;
    }
    if (!validate_number_range(config.control.dropDetentClosed, 0.f, 180.f)) {
        snprintf(error, error_size, "Drop detent (closed) must be between 0 and 180 degrees.");
        return false;
    }
    if (!validate_number_range(config.control.dropDetentOpen, 0.f, 180.f)) {
        snprintf(error, error_size, "Drop detent (open) must be between 0 and 180 degrees.");
        return false;
    }
    switch ((ControlMode)config.general.controlMode) {
        case CTRLMODE_3AXIS_ATHR:
        case CTRLMODE_3AXIS:
        case CTRLMODE_2AXIS_ATHR:
        case CTRLMODE_2AXIS:
            if (!validate_number_range(config.control.maxAilDeflection, 0.f, 90.f)) {
                snprintf(error, error_size, "Max aileron deflection must be between 0 and 90 degrees.");
                return false;
            }
            if (!validate_number_range(config.control.maxEleDeflection, 0.f, 90.f)) {
                snprintf(error, error_size, "Max elevator deflection must be between 0 and 90 degrees.");
                return false;
            }
            if (!validate_number_range(config.control.maxRudDeflection, 0.f, 90.f)) {
                snprintf(error, error_size, "Max rudder deflection must be between 0 and 90 degrees.");
                return false;
            }
            break;
        case CTRLMODE_FLYINGWING_ATHR:
        case CTRLMODE_FLYINGWING:
            if (!validate_number_range(config.control.maxElevonDeflection, 0.f, 90.f)) {
                snprintf(error, error_size, "Max elevon deflection must be between 0 and 90 degrees.");
                return false;
            }
            break;
    }
    if (strlen(config.system.ssid) < WIFI_SSID_MIN_LEN || strlen(config.system.ssid) > WIFI_SSID_MAX_LEN) {
        snprintf(error, error_size, "Wi-Fi SSID must be between %d and %d characters.", WIFI_SSID_MIN_LEN,
                 WIFI_SSID_MAX_LEN);
        return false;
    }
    if (strlen(config.system.pass) > 0 &&
        (strlen(config.system.pass) < WIFI_PASS_MIN_LEN || strlen(config.system.pass) > WIFI_PASS_MAX_LEN)) {
        snprintf(error, error_size, "Wi-Fi password must be between %d and %d characters.", WIFI_PASS_MIN_LEN,
                 WIFI_PASS_MAX_LEN);
        return false;
    }
    return true;
}
