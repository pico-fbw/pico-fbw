/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "pico/types.h"

#include "config.h"

Config config;

/* Begin the world's grossest code...
Look, the readability of the config is really good everywhere BUT here. It's a compromise I was willing to make.
Just ignore this file and we'll all be happy, okay? Okay. */

// Helper function to check if a value occurs more than once in an array
bool contains_multiple(uint array[], uint size, uint value) {
    uint count = 0;
    for (uint i = 0; i < size; i++) {
        if (array[i] == value) {
            count++;
        }
    }
    return count > 1;
}

/**
 * Helper function to check if all pins provided are unique.
 * @param num_pins number of pins provided
 * @param ... pins to check (casted to uints)
 * @return true if all pins are unique, false otherwise
*/
bool pins_unique(uint num_pins, ...) {
    va_list args;
    va_start(args, num_pins);
    // Initialize an array to store pin numbers, variadic doesn't support these directly
    uint pins[num_pins];
    for (uint i = 0; i < num_pins; i++) {
        pins[i] = va_arg(args, uint);
    }
    va_end(args);
    for (uint i = 0; i < num_pins; i++) {
        if (contains_multiple(pins, num_pins, pins[i])) {
            return false;
        }
    }
    return true;
}

/**
 * Validates current RAM-banked config values.
 * @return true if config is valid, false if invalid
*/
static bool validateConfig() {
    // Unique pin validation
    switch (config.general.controlMode) {
        case CTRLMODE_3AXIS_ATHR:
            if (!pins_unique(13, config.pins0.inputAil, config.pins0.inputElev, config.pins0.inputRud, config.pins0.inputSwitch,
            config.pins0.servoAil, config.pins0.servoElev, config.pins0.servoRud, config.pins1.inputThrottle, config.pins1.escThrottle,
            config.sensors.imuSda, config.sensors.imuScl, config.sensors.gpsRx, config.sensors.gpsTx)) goto invalid;
            break;
        case CTRLMODE_3AXIS:
            if (!pins_unique(11, config.pins0.inputAil, config.pins0.inputElev, config.pins0.inputRud, config.pins0.inputSwitch,
            config.pins0.servoAil, config.pins0.servoElev, config.pins0.servoRud, config.sensors.imuSda, config.sensors.imuScl,
            config.sensors.gpsRx, config.sensors.gpsTx)) goto invalid;
            break;
        case CTRLMODE_FLYINGWING_ATHR:
            if (!pins_unique(12, config.pins0.inputAil, config.pins0.inputElev, config.pins0.inputRud, config.pins0.inputSwitch,
            config.pins1.servoElevonL, config.pins1.servoElevonR, config.pins1.inputThrottle, config.pins1.escThrottle,
            config.sensors.imuSda, config.sensors.imuScl, config.sensors.gpsRx, config.sensors.gpsTx)) goto invalid;
            break;
        case CTRLMODE_FLYINGWING:
            if (!pins_unique(10, config.pins0.inputAil, config.pins0.inputElev, config.pins0.inputRud, config.pins0.inputSwitch, config.pins1.servoElevonL,
            config.pins1.servoElevonR, config.sensors.imuSda, config.sensors.imuScl, config.sensors.gpsRx, config.sensors.gpsTx)) goto invalid;
            break;
        invalid:
            if (config.debug.debug_fbw) printf("ERROR: A pin may only be used once.\n");
            return false;
    }
    // Sensor pin validation
    // IMU_SDA can be on pins 0, 4, 8, 12, 16, 20, 28
    if (config.sensors.imuSda != 0 && config.sensors.imuSda != 4 && config.sensors.imuSda != 8 && config.sensors.imuSda != 12 &&
    config.sensors.imuSda != 16 && config.sensors.imuSda != 20 && config.sensors.imuSda != 28) {
        if (config.debug.debug_fbw) printf("ERROR: IMU_SDA must be on the I2C0_SDA interface.\n");
        return false;
    }
    // IMU_SCL can be on pins 1, 5, 9, 13, 17, 21
    if (config.sensors.imuScl != 1 && config.sensors.imuScl != 5 && config.sensors.imuScl != 9 && config.sensors.imuScl != 13 &&
    config.sensors.imuScl != 17 && config.sensors.imuScl != 21) {
        if (config.debug.debug_fbw) printf("ERROR: IMU_SCL must be on the I2C0_SCL interface.\n");
        return false;
    }
    // GPS_RX can be on pins 4, 8, 20
    if (config.sensors.gpsRx != 4 && config.sensors.gpsRx != 8 && config.sensors.gpsRx != 20) {
        if (config.debug.debug_fbw) printf("ERROR: GPS_RX must be on the UART1_RX interface.\n");
        return false;
    }
    // GPS_TX can be on pins 5, 9, 21
    if (config.sensors.gpsTx != 5 && config.sensors.gpsTx != 9 && config.sensors.gpsTx != 21) {
        if (config.debug.debug_fbw) printf("ERROR: GPS_TX must be on the UART1_TX interface.\n");
        return false;
    }
    // Limit validation
    if (config.limits.rollLimit > 72 ||  config.limits.rollLimit < 0) {
        if (config.debug.debug_fbw) printf("ERROR: Roll limit must be between 0 and 72 degrees.\n");
        return false;
    }
    if (config.limits.rollLimitHold > 72 || config.limits.rollLimitHold < 0) {
        if (config.debug.debug_fbw) printf("ERROR: Roll limit hold must be between 0 and 72 degrees.\n");
        return false;
    }
    if (config.limits.pitchUpperLimit > 35 || config.limits.pitchUpperLimit < 0) {
        if (config.debug.debug_fbw) printf("ERROR: Upper pitch limit must be between 0 and 35 degrees.\n");
        return false;
    }
    if (config.limits.pitchLowerLimit < -20 || config.limits.pitchLowerLimit > 0) {
        if (config.debug.debug_fbw) printf("ERROR: Lower pitch limit must be between -20 and 0 degrees.\n");
        return false;
    }
    switch (config.general.controlMode) {
        case CTRLMODE_3AXIS_ATHR:
        case CTRLMODE_3AXIS:
            if (config.limits.maxAilDeflection > 90 || config.limits.maxAilDeflection < 0) {
                if (config.debug.debug_fbw) printf("ERROR: Max aileron deflection must be between 0 and 90 degrees.\n");
                return false;
            }
            if (config.limits.maxElevDeflection > 90 || config.limits.maxElevDeflection < 0) {
                if (config.debug.debug_fbw) printf("ERROR: Max elevator deflection must be between 0 and 90 degrees.\n");
                return false;
            }
            if (config.limits.maxRudDeflection > 90 || config.limits.maxRudDeflection < 0) {
                if (config.debug.debug_fbw) printf("ERROR: Max rudder deflection must be between 0 and 90 degrees.\n");
                return false;
            }
            break;
        case CTRLMODE_FLYINGWING_ATHR:
        case CTRLMODE_FLYINGWING:
            if (config.limits.maxElevonDeflection > 90 || config.limits.maxElevonDeflection < 0) {
                if (config.debug.debug_fbw) printf("ERROR: Max elevon deflection must be between 0 and 90 degrees.\n");
                return false;
            }
            break;
    }
    return true;
}

bool config_load(ConfigSource source) {
    switch (source) {
        case FROM_FLASH:
            // Validate that flash sectors have been written to before loading
            for (FloatSector s = FLOAT_SECTOR_MIN_CONFIG; s <= FLOAT_SECTOR_MAX_CONFIG; s++) {
                for (uint v = 0; v < FLOAT_SECTOR_SIZE; v++) {
                    if (!isfinite(flash_readFloat(s, v))) return false;
                }
            }

            // ConfigGeneral
            config.general.controlMode = (ControlMode)flash_readFloat(FLOAT_SECTOR_CONFIG_GENERAL, 0);
            config.general.switchType = (SwitchType)flash_readFloat(FLOAT_SECTOR_CONFIG_GENERAL, 1);
            config.general.maxCalibrationOffset = (uint8_t)flash_readFloat(FLOAT_SECTOR_CONFIG_GENERAL, 2);
            config.general.servoHz = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_GENERAL, 3);
            config.general.escHz = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_GENERAL, 4);
            config.general.apiEnabled = (bool)flash_readFloat(FLOAT_SECTOR_CONFIG_GENERAL, 5);
            config.general.wiflyUsePass = (bool)flash_readFloat(FLOAT_SECTOR_CONFIG_GENERAL, 6);
            config.general.skipCalibration = (bool)flash_readFloat(FLOAT_SECTOR_CONFIG_GENERAL, 7);

            // ConfigControl
            config.control.controlSensitivity = flash_readFloat(FLOAT_SECTOR_CONFIG_CONTROL, 0);
            config.control.rudderSensitivity = flash_readFloat(FLOAT_SECTOR_CONFIG_CONTROL, 1);
            config.control.controlDeadband = flash_readFloat(FLOAT_SECTOR_CONFIG_CONTROL, 2);
            config.control.throttleDetentIdle = flash_readFloat(FLOAT_SECTOR_CONFIG_CONTROL, 3);
            config.control.throttleDetentMCT = flash_readFloat(FLOAT_SECTOR_CONFIG_CONTROL, 4);
            config.control.throttleDetentMax = flash_readFloat(FLOAT_SECTOR_CONFIG_CONTROL, 5);
            config.control.throttleMaxTime = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_CONTROL, 6);

            // ConfigLimits
            config.limits.rollLimit = flash_readFloat(FLOAT_SECTOR_CONFIG_LIMITS, 0);
            config.limits.rollLimitHold = flash_readFloat(FLOAT_SECTOR_CONFIG_LIMITS, 1);
            config.limits.pitchUpperLimit = flash_readFloat(FLOAT_SECTOR_CONFIG_LIMITS, 2);
            config.limits.pitchLowerLimit = flash_readFloat(FLOAT_SECTOR_CONFIG_LIMITS, 3);
            config.limits.maxAilDeflection = flash_readFloat(FLOAT_SECTOR_CONFIG_LIMITS, 4);
            config.limits.maxElevDeflection = flash_readFloat(FLOAT_SECTOR_CONFIG_LIMITS, 5);
            config.limits.maxRudDeflection = flash_readFloat(FLOAT_SECTOR_CONFIG_LIMITS, 6);
            config.limits.maxElevonDeflection = flash_readFloat(FLOAT_SECTOR_CONFIG_LIMITS, 7);

            // ConfigFlyingWing
            config.flyingWing.elevonMixingGain = flash_readFloat(FLOAT_SECTOR_CONFIG_FLYINGWING, 0);
            config.flyingWing.ailMixingBias = flash_readFloat(FLOAT_SECTOR_CONFIG_FLYINGWING, 1);
            config.flyingWing.elevMixingBias = flash_readFloat(FLOAT_SECTOR_CONFIG_FLYINGWING, 2);

            // ConfigPins0
            config.pins0.inputAil = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_PINS0, 0);
            config.pins0.servoAil = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_PINS0, 1);
            config.pins0.inputElev = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_PINS0, 2);
            config.pins0.servoElev = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_PINS0, 3);
            config.pins0.inputRud = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_PINS0, 4);
            config.pins0.servoRud = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_PINS0, 5);
            config.pins0.inputSwitch = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_PINS0, 6);

            // ConfigPins1
            config.pins1.inputThrottle = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_PINS1, 0);
            config.pins1.escThrottle = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_PINS1, 1);
            config.pins1.servoElevonL = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_PINS1, 2);
            config.pins1.servoElevonR = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_PINS1, 3);
            config.pins1.reverseRoll = (bool)flash_readFloat(FLOAT_SECTOR_CONFIG_PINS1, 4);
            config.pins1.reversePitch = (bool)flash_readFloat(FLOAT_SECTOR_CONFIG_PINS1, 5);
            config.pins1.reverseYaw = (bool)flash_readFloat(FLOAT_SECTOR_CONFIG_PINS1, 6);

            // ConfigSensors
            config.sensors.imuModel = (IMUModel)flash_readFloat(FLOAT_SECTOR_CONFIG_SENSORS, 0);
            config.sensors.imuSda = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_SENSORS, 1);
            config.sensors.imuScl = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_SENSORS, 2);
            config.sensors.gpsEnabled = (bool)flash_readFloat(FLOAT_SECTOR_CONFIG_SENSORS, 3);
            config.sensors.gpsBaudrate = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_SENSORS, 4);
            config.sensors.gpsCommandType = (GPSCommandType)flash_readFloat(FLOAT_SECTOR_CONFIG_SENSORS, 5);
            config.sensors.gpsTx = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_SENSORS, 6);
            config.sensors.gpsRx = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_SENSORS, 7);

            // ConfigPID0
            config.pid0.rollTau = flash_readFloat(FLOAT_SECTOR_CONFIG_PID0, 0);
            config.pid0.rollIntegMin = flash_readFloat(FLOAT_SECTOR_CONFIG_PID0, 1);
            config.pid0.rollIntegMax = flash_readFloat(FLOAT_SECTOR_CONFIG_PID0, 2);
            config.pid0.rollKt = flash_readFloat(FLOAT_SECTOR_CONFIG_PID0, 3);
            config.pid0.pitchTau = flash_readFloat(FLOAT_SECTOR_CONFIG_PID0, 4);
            config.pid0.pitchIntegMin = flash_readFloat(FLOAT_SECTOR_CONFIG_PID0, 5);
            config.pid0.pitchIntegMax = flash_readFloat(FLOAT_SECTOR_CONFIG_PID0, 6);
            config.pid0.pitchKt = flash_readFloat(FLOAT_SECTOR_CONFIG_PID0, 7);

            // ConfigPID1
            config.pid1.yawKp = flash_readFloat(FLOAT_SECTOR_CONFIG_PID1, 0);
            config.pid1.yawKi = flash_readFloat(FLOAT_SECTOR_CONFIG_PID1, 1);
            config.pid1.yawKd = flash_readFloat(FLOAT_SECTOR_CONFIG_PID1, 2);
            config.pid1.yawTau = flash_readFloat(FLOAT_SECTOR_CONFIG_PID1, 3);
            config.pid1.yawIntegMin = flash_readFloat(FLOAT_SECTOR_CONFIG_PID1, 4);
            config.pid1.yawIntegMax = flash_readFloat(FLOAT_SECTOR_CONFIG_PID1, 5);
            config.pid1.yawKt = flash_readFloat(FLOAT_SECTOR_CONFIG_PID1, 6);

            // ConfigDebug
            #if defined(LIB_PICO_STDIO_USB) || defined(LIB_PICO_STDIO_UART)
                config.debug.debug = true;
                config.debug.debug_fbw = (bool)flash_readFloat(FLOAT_SECTOR_CONFIG_DEBUG, 1);
                config.debug.debug_imu = (bool)flash_readFloat(FLOAT_SECTOR_CONFIG_DEBUG, 2);
                config.debug.debug_gps = (bool)flash_readFloat(FLOAT_SECTOR_CONFIG_DEBUG, 3);
                config.debug.debug_wifly = (bool)flash_readFloat(FLOAT_SECTOR_CONFIG_DEBUG, 4);
                config.debug.debug_network = (bool)flash_readFloat(FLOAT_SECTOR_CONFIG_DEBUG, 5);
                config.debug.dump_network = (bool)flash_readFloat(FLOAT_SECTOR_CONFIG_DEBUG, 6);
                config.debug.watchdog_timeout_ms = (uint32_t)flash_readFloat(FLOAT_SECTOR_CONFIG_DEBUG, 7);
            #else
                // All debugging is automatically disabled when stdio is not enabled
                config.debug.debug = false;
                config.debug.debug_fbw = false;
                config.debug.debug_imu = false;
                config.debug.debug_gps = false;
                config.debug.debug_wifly = false;
                config.debug.debug_network = false;
                config.debug.dump_network = false;
            #endif

            // ConfigWifly
            const char* ssid = flash_readString(STRING_SECTOR_CONFIG_WIFLY_SSID);
            if (ssid) {
                strncpy(config.wifly.ssid, ssid, STRING_SECTOR_SIZE);
            } else {
                return false;
            }
            const char* pass = flash_readString(STRING_SECTOR_CONFIG_WIFLY_PASS);
            if (pass) {
                strncpy(config.wifly.pass, pass, STRING_SECTOR_SIZE);
            } else {
                return false;
            }

            break;
        case DEFAULT_VALUES:
            // Very similar except load from DEFault macros instead of flash (and thus no validation)
            // ConfigGeneral
            config.general.controlMode = CONTROL_MODE_DEF;
            config.general.switchType = SWITCH_TYPE_DEF;
            config.general.maxCalibrationOffset = MAX_CALIBRATION_OFFSET_DEF;
            config.general.servoHz = SERVO_HZ_DEF;
            config.general.escHz = ESC_HZ_DEF;
            config.general.apiEnabled = API_ENABLED_DEF;
            config.general.wiflyUsePass = WIFLY_USE_PASS_DEF;
            config.general.skipCalibration = SKIP_CALIBRATION_DEF;

            // ConfigControl
            config.control.controlSensitivity = CONTROL_SENSITIVITY_DEF;
            config.control.rudderSensitivity = RUDDER_SENSITIVITY_DEF;
            config.control.controlDeadband = CONTROL_DEADBAND_DEF;
            config.control.throttleDetentIdle = THROTTLE_DETENT_IDLE_DEF;
            config.control.throttleDetentMCT = THROTTLE_DETENT_MCT_DEF;
            config.control.throttleDetentMax = THROTTLE_DETENT_MAX_DEF;
            config.control.throttleMaxTime = THROTTLE_MAX_TIME_DEF;

            // ConfigLimits
            config.limits.rollLimit = ROLL_LIMIT_DEF;
            config.limits.rollLimitHold = ROLL_LIMIT_HOLD_DEF;
            config.limits.pitchUpperLimit = PITCH_UPPER_LIMIT_DEF;
            config.limits.pitchLowerLimit = PITCH_LOWER_LIMIT_DEF;
            config.limits.maxAilDeflection = MAX_AIL_DEFLECTION_DEF;
            config.limits.maxElevDeflection = MAX_ELEV_DEFLECTION_DEF;
            config.limits.maxRudDeflection = MAX_RUD_DEFLECTION_DEF;
            config.limits.maxElevonDeflection = MAX_ELEVON_DEFLECTION_DEF;

            // ConfigFlyingWing
            config.flyingWing.elevonMixingGain = ELEVON_MIXING_GAIN_DEF;
            config.flyingWing.ailMixingBias = AIL_MIXING_BIAS_DEF;
            config.flyingWing.elevMixingBias = ELEV_MIXING_BIAS_DEF;

            // ConfigPins0
            config.pins0.inputAil = INPUT_AIL_DEF;
            config.pins0.servoAil = SERVO_AIL_DEF;
            config.pins0.inputElev = INPUT_ELEV_DEF;
            config.pins0.servoElev = SERVO_ELEV_DEF;
            config.pins0.inputRud = INPUT_RUD_DEF;
            config.pins0.servoRud = SERVO_RUD_DEF;
            config.pins0.inputSwitch = INPUT_SWITCH_DEF;

            // ConfigPins1
            config.pins1.inputThrottle = INPUT_THROTTLE_DEF;
            config.pins1.escThrottle = ESC_THROTTLE_DEF;
            config.pins1.servoElevonL = SERVO_ELEVON_L_DEF;
            config.pins1.servoElevonR = SERVO_ELEVON_R_DEF;
            config.pins1.reverseRoll = REVERSE_ROLL_DEF;
            config.pins1.reversePitch = REVERSE_PITCH_DEF;
            config.pins1.reverseYaw = REVERSE_YAW_DEF;

            // ConfigSensors
            config.sensors.imuModel = IMU_MODEL_DEF;
            config.sensors.imuSda = IMU_SDA_DEF;
            config.sensors.imuScl = IMU_SCL_DEF;
            config.sensors.gpsEnabled = GPS_ENABLED_DEF;
            config.sensors.gpsBaudrate = GPS_BAUDRATE_DEF;
            config.sensors.gpsCommandType = GPS_COMMAND_TYPE_DEF;
            config.sensors.gpsTx = GPS_TX_DEF;
            config.sensors.gpsRx = GPS_RX_DEF;

            // ConfigPID0
            config.pid0.rollTau = ROLL_TAU_DEF;
            config.pid0.rollIntegMin = ROLL_INTEG_MIN_DEF;
            config.pid0.rollIntegMax = ROLL_INTEG_MAX_DEF;
            config.pid0.rollKt = ROLL_KT_DEF;
            config.pid0.pitchTau = PITCH_TAU_DEF;
            config.pid0.pitchIntegMin = PITCH_INTEG_MIN_DEF;
            config.pid0.pitchIntegMax = PITCH_INTEG_MAX_DEF;
            config.pid0.pitchKt = PITCH_KT_DEF;

            // ConfigPID1
            config.pid1.yawKp = YAW_KP_DEF;
            config.pid1.yawKi = YAW_KI_DEF;
            config.pid1.yawKd = YAW_KD_DEF;
            config.pid1.yawTau = YAW_TAU_DEF;
            config.pid1.yawIntegMin = YAW_INTEG_MIN_DEF;
            config.pid1.yawIntegMax = YAW_INTEG_MAX_DEF;
            config.pid1.yawKt = YAW_KT_DEF;

            // ConfigDebug
            #if defined(LIB_PICO_STDIO_USB) || defined(LIB_PICO_STDIO_UART)
                config.debug.debug = true;
                config.debug.debug_fbw = DEBUG_FBW_DEF;
                config.debug.debug_imu = DEBUG_IMU_DEF;
                config.debug.debug_gps = DEBUG_GPS_DEF;
                config.debug.debug_wifly = DEBUG_WIFLY_DEF;
                config.debug.debug_network = DEBUG_NETWORK_DEF;
                config.debug.dump_network = DUMP_NETWORK_DEF;
            #else
                config.debug.debug = false;
                config.debug.debug_fbw = false;
                config.debug.debug_imu = false;
                config.debug.debug_gps = false;
                config.debug.debug_wifly = false;
                config.debug.debug_network = false;
                config.debug.dump_network = false;
            #endif
            config.debug.watchdog_timeout_ms = WATCHDOG_TIMEOUT_MS_DEF;
            // ConfigWifly
            strncpy(config.wifly.ssid, WIFLY_SSID_DEF, STRING_SECTOR_SIZE);
            strncpy(config.wifly.pass, WIFLY_PASS_DEF, STRING_SECTOR_SIZE);

            break;
    }
    return true;
}

bool config_save(bool validate) {
    // Validate config before saving
    if (validate) {
        if (!validateConfig()) {
            if (config.debug.debug_fbw) printf("Config validation failed, config will not be saved!\n");
            return false;
        }
    }

    // ConfigGeneral
    float general[FLOAT_SECTOR_SIZE] = {
        config.general.controlMode,
        config.general.switchType,
        config.general.maxCalibrationOffset,
        config.general.servoHz,
        config.general.escHz,
        config.general.apiEnabled,
        config.general.wiflyUsePass,
        config.general.skipCalibration
    }; // C already casts all elements in the array to a float so there's no need to explicitly cast them
    flash_writeFloat(FLOAT_SECTOR_CONFIG_GENERAL, general);

    // ConfigControl
    float control[FLOAT_SECTOR_SIZE] = {
        config.control.controlSensitivity,
        config.control.rudderSensitivity,
        config.control.controlDeadband,
        config.control.throttleDetentIdle,
        config.control.throttleDetentMCT,
        config.control.throttleDetentMax,
        config.control.throttleMaxTime
    };
    flash_writeFloat(FLOAT_SECTOR_CONFIG_CONTROL, control);

    // ConfigLimits
    float limits[FLOAT_SECTOR_SIZE] = {
        config.limits.rollLimit,
        config.limits.rollLimitHold,
        config.limits.pitchUpperLimit,
        config.limits.pitchLowerLimit,
        config.limits.maxAilDeflection,
        config.limits.maxElevDeflection,
        config.limits.maxRudDeflection,
        config.limits.maxElevonDeflection
    };
    flash_writeFloat(FLOAT_SECTOR_CONFIG_LIMITS, limits);

    // ConfigFlyingWing
    float flyingWing[FLOAT_SECTOR_SIZE] = {
        config.flyingWing.elevonMixingGain,
        config.flyingWing.ailMixingBias,
        config.flyingWing.elevMixingBias
    };
    flash_writeFloat(FLOAT_SECTOR_CONFIG_FLYINGWING, flyingWing);

    // ConfigPins0
    float pins0[FLOAT_SECTOR_SIZE] = {
        config.pins0.inputAil,
        config.pins0.servoAil,
        config.pins0.inputElev,
        config.pins0.servoElev,
        config.pins0.inputRud,
        config.pins0.servoRud,
        config.pins0.inputSwitch
    };
    flash_writeFloat(FLOAT_SECTOR_CONFIG_PINS0, pins0);

    // ConfigPins1
    float pins1[FLOAT_SECTOR_SIZE] = {
        config.pins1.inputThrottle,
        config.pins1.escThrottle,
        config.pins1.servoElevonL,
        config.pins1.servoElevonR,
        config.pins1.reverseRoll,
        config.pins1.reversePitch,
        config.pins1.reverseYaw
    };
    flash_writeFloat(FLOAT_SECTOR_CONFIG_PINS1, pins1);

    // ConfigSensors
    float sensors[FLOAT_SECTOR_SIZE] = {
        config.sensors.imuModel,
        config.sensors.imuSda,
        config.sensors.imuScl,
        config.sensors.gpsEnabled,
        config.sensors.gpsBaudrate,
        config.sensors.gpsCommandType,
        config.sensors.gpsTx,
        config.sensors.gpsRx
    };
    flash_writeFloat(FLOAT_SECTOR_CONFIG_SENSORS, sensors);

    // ConfigPID0
    float rollPitchPID[FLOAT_SECTOR_SIZE] = {
        config.pid0.rollTau,
        config.pid0.rollIntegMin,
        config.pid0.rollIntegMax,
        config.pid0.rollKt,
        config.pid0.pitchTau,
        config.pid0.pitchIntegMin,
        config.pid0.pitchIntegMax,
        config.pid0.pitchKt
    };
    flash_writeFloat(FLOAT_SECTOR_CONFIG_PID0, rollPitchPID);

    // ConfigPID1
    float yawPID[FLOAT_SECTOR_SIZE] = {
        config.pid1.yawKp,
        config.pid1.yawKi,
        config.pid1.yawKd,
        config.pid1.yawTau,
        config.pid1.yawIntegMin,
        config.pid1.yawIntegMax,
        config.pid1.yawKt
    };
    flash_writeFloat(FLOAT_SECTOR_CONFIG_PID1, yawPID);

    // ConfigDebug
    float debug[FLOAT_SECTOR_SIZE] = {
        config.debug.debug,
        config.debug.debug_fbw,
        config.debug.debug_imu,
        config.debug.debug_gps,
        config.debug.debug_wifly,
        config.debug.debug_network,
        config.debug.dump_network,
        config.debug.watchdog_timeout_ms
    };
    flash_writeFloat(FLOAT_SECTOR_CONFIG_DEBUG, debug);
    
    // ConfigWifly -- uses StringsSectors not floats
    flash_writeString(STRING_SECTOR_CONFIG_WIFLY_SSID, config.wifly.ssid);
    flash_writeString(STRING_SECTOR_CONFIG_WIFLY_PASS, config.wifly.pass);

    return true;
}

ConfigSectionType config_getSectionType(const char *section) {
    if (strcasecmp(section, CONFIG_GENERAL_STR) == 0) {
        return SECTION_TYPE_FLOAT;
    } else if (strcasecmp(section, CONFIG_CONTROL_STR) == 0) {
        return SECTION_TYPE_FLOAT;
    } else if (strcasecmp(section, CONFIG_LIMITS_STR) == 0) {
        return SECTION_TYPE_FLOAT;
    } else if (strcasecmp(section, CONFIG_FLYING_WING_STR) == 0) {
        return SECTION_TYPE_FLOAT;
    } else if (strcasecmp(section, CONFIG_PINS0_STR) == 0) {
        return SECTION_TYPE_FLOAT;
    } else if (strcasecmp(section, CONFIG_PINS1_STR) == 0) {
        return SECTION_TYPE_FLOAT;
    } else if (strcasecmp(section, CONFIG_SENSORS_STR) == 0) {
        return SECTION_TYPE_FLOAT;
    } else if (strcasecmp(section, CONFIG_PID0_STR) == 0) {
        return SECTION_TYPE_FLOAT;
    } else if (strcasecmp(section, CONFIG_PID1_STR) == 0) {
        return SECTION_TYPE_FLOAT;
    } else if (strcasecmp(section, CONFIG_DEBUG_STR) == 0) {
        return SECTION_TYPE_FLOAT;
    } else if (strcasecmp(section, CONFIG_WIFLY_STR) == 0) {
        return SECTION_TYPE_STRING;
    } else {
        return SECTION_TYPE_NONE;
    }
}

const char *config_sectionToString(ConfigSection section) {
    switch (section) {
        case CONFIG_GENERAL:
            return CONFIG_GENERAL_STR;
        case CONFIG_CONTROL:
            return CONFIG_CONTROL_STR;
        case CONFIG_LIMITS:
            return CONFIG_LIMITS_STR;
        case CONFIG_FLYING_WING:
            return CONFIG_FLYING_WING_STR;
        case CONFIG_PINS0:
            return CONFIG_PINS0_STR;
        case CONFIG_PINS1:
            return CONFIG_PINS1_STR;
        case CONFIG_SENSORS:
            return CONFIG_SENSORS_STR;
        case CONFIG_PID0:
            return CONFIG_PID0_STR;
        case CONFIG_PID1:
            return CONFIG_PINS1_STR;
        case CONFIG_DEBUG:
            return CONFIG_DEBUG_STR;
        case CONFIG_WIFLY:
            return CONFIG_WIFLY_STR;
        default:
            return NULL;
    }
}

float config_getFloat(const char* section, const char* key) {
    if (strcasecmp(section, CONFIG_GENERAL_STR) == 0) {
        if (strcasecmp(key, "controlMode") == 0) {
            return (float)config.general.controlMode;
        } else if (strcasecmp(key, "switchType") == 0) {
            return (float)config.general.switchType;
        } else if (strcasecmp(key, "maxCalibrationOffset") == 0) {
            return (float)config.general.maxCalibrationOffset;
        } else if (strcasecmp(key, "servoHz") == 0) {
            return (float)config.general.servoHz;
        } else if (strcasecmp(key, "escHz") == 0) {
            return (float)config.general.escHz;
        } else if (strcasecmp(key, "apiEnabled") == 0) {
            return (float)config.general.apiEnabled;
        } else if (strcasecmp(key, "wiflyUsePass") == 0) {
            return (float)config.general.wiflyUsePass;
        } else if (strcasecmp(key, "skipCalibration") == 0) {
            return (float)config.general.skipCalibration;
        }
    } else if (strcasecmp(section, CONFIG_CONTROL_STR) == 0) {
        if (strcasecmp(key, "controlSensitivity") == 0) {
            return config.control.controlSensitivity;
        } else if (strcasecmp(key, "rudderSensitivity") == 0) {
            return config.control.rudderSensitivity;
        } else if (strcasecmp(key, "controlDeadband") == 0) {
            return config.control.controlDeadband;
        } else if (strcasecmp(key, "throttleDetentIdle") == 0) {
            return config.control.throttleDetentIdle;
        } else if (strcasecmp(key, "throttleDetentMCT") == 0) {
            return config.control.throttleDetentMCT;
        } else if (strcasecmp(key, "throttleDetentMax") == 0) {
            return config.control.throttleDetentMax;
        } else if (strcasecmp(key, "throttleMaxTime") == 0) {
            return (float)config.control.throttleMaxTime;
        }
    } else if (strcasecmp(section, CONFIG_LIMITS_STR) == 0) {
        if (strcasecmp(key, "rollLimit") == 0) {
            return config.limits.rollLimit;
        } else if (strcasecmp(key, "rollLimitHold") == 0) {
            return config.limits.rollLimitHold;
        } else if (strcasecmp(key, "pitchUpperLimit") == 0) {
            return config.limits.pitchUpperLimit;
        } else if (strcasecmp(key, "pitchLowerLimit") == 0) {
            return config.limits.pitchLowerLimit;
        } else if (strcasecmp(key, "maxAilDeflection") == 0) {
            return config.limits.maxAilDeflection;
        } else if (strcasecmp(key, "maxElevDeflection") == 0) {
            return config.limits.maxElevDeflection;
        } else if (strcasecmp(key, "maxRudDeflection") == 0) {
            return config.limits.maxRudDeflection;
        } else if (strcasecmp(key, "maxElevonDeflection") == 0) {
            return config.limits.maxElevonDeflection;
        }
    } else if (strcasecmp(section, CONFIG_FLYING_WING_STR) == 0) {
        if (strcasecmp(key, "elevonMixingGain") == 0) {
            return config.flyingWing.elevonMixingGain;
        } else if (strcasecmp(key, "ailMixingBias") == 0) {
            return config.flyingWing.ailMixingBias;
        } else if (strcasecmp(key, "elevMixingBias") == 0) {
            return config.flyingWing.elevMixingBias;
        }
    } else if (strcasecmp(section, CONFIG_PINS0_STR) == 0) {
        if (strcasecmp(key, "inputAil") == 0) {
            return (float)config.pins0.inputAil;
        } else if (strcasecmp(key, "servoAil") == 0) {
            return (float)config.pins0.servoAil;
        } else if (strcasecmp(key, "inputElev") == 0) {
            return (float)config.pins0.inputElev;
        } else if (strcasecmp(key, "servoElev") == 0) {
            return (float)config.pins0.servoElev;
        } else if (strcasecmp(key, "inputRud") == 0) {
            return (float)config.pins0.inputRud;
        } else if (strcasecmp(key, "servoRud") == 0) {
            return (float)config.pins0.servoRud;
        } else if (strcasecmp(key, "inputSwitch") == 0) {
            return (float)config.pins0.inputSwitch;
        }
    } else if (strcasecmp(section, CONFIG_PINS1_STR) == 0) {
        if (strcasecmp(key, "inputThrottle") == 0) {
            return (float)config.pins1.inputThrottle;
        } else if (strcasecmp(key, "escThrottle") == 0) {
            return (float)config.pins1.escThrottle;
        } else if (strcasecmp(key, "servoElevonL") == 0) {
            return (float)config.pins1.servoElevonL;
        } else if (strcasecmp(key, "servoElevonR") == 0) {
            return (float)config.pins1.servoElevonR;
        } else if (strcasecmp(key, "reverseRoll") == 0) {
            return (float)config.pins1.reverseRoll;
        } else if (strcasecmp(key, "reversePitch") == 0) {
            return (float)config.pins1.reversePitch;
        } else if (strcasecmp(key, "reverseYaw") == 0) {
            return (float)config.pins1.reverseYaw;
        }
    } else if (strcasecmp(section, CONFIG_SENSORS_STR) == 0) {
        if (strcasecmp(key, "imuModel") == 0) {
            return (float)config.sensors.imuModel;
        } else if (strcasecmp(key, "imuSda") == 0) {
            return (float)config.sensors.imuSda;
        } else if (strcasecmp(key, "imuScl") == 0) {
            return (float)config.sensors.imuScl;
        } else if (strcasecmp(key, "gpsEnabled") == 0) {
            return (float)config.sensors.gpsEnabled;
        } else if (strcasecmp(key, "gpsBaudrate") == 0) {
            return (float)config.sensors.gpsBaudrate;
        } else if (strcasecmp(key, "gpsCommandType") == 0) {
            return (float)config.sensors.gpsCommandType;
        } else if (strcasecmp(key, "gpsTx") == 0) {
            return (float)config.sensors.gpsTx;
        } else if (strcasecmp(key, "gpsRx") == 0) {
            return (float)config.sensors.gpsRx;
        }
    } else if (strcasecmp(section, CONFIG_PID0_STR) == 0) {
        if (strcasecmp(key, "rollTau") == 0) {
            return config.pid0.rollTau;
        } else if (strcasecmp(key, "rollIntegMin") == 0) {
            return config.pid0.rollIntegMin;
        } else if (strcasecmp(key, "rollIntegMax") == 0) {
            return config.pid0.rollIntegMax;
        } else if (strcasecmp(key, "rollKt") == 0) {
            return config.pid0.rollKt;
        } else if (strcasecmp(key, "pitchTau") == 0) {
            return config.pid0.pitchTau;
        } else if (strcasecmp(key, "pitchIntegMin") == 0) {
            return config.pid0.pitchIntegMin;
        } else if (strcasecmp(key, "pitchIntegMax") == 0) {
            return config.pid0.pitchIntegMax;
        } else if (strcasecmp(key, "pitchKt") == 0) {
            return config.pid0.pitchKt;
        }
    } else if (strcasecmp(section, CONFIG_PID1_STR) == 0) {
        if (strcasecmp(key, "yawKp") == 0) {
            return config.pid1.yawKp;
        } else if (strcasecmp(key, "yawKi") == 0) {
            return config.pid1.yawKi;
        } else if (strcasecmp(key, "yawKd") == 0) {
            return config.pid1.yawKd;
        } else if (strcasecmp(key, "yawTau") == 0) {
            return config.pid1.yawTau;
        } else if (strcasecmp(key, "yawIntegMin") == 0) {
            return config.pid1.yawIntegMin;
        } else if (strcasecmp(key, "yawIntegMax") == 0) {
            return config.pid1.yawIntegMax;
        } else if (strcasecmp(key, "yawKt") == 0) {
            return config.pid1.yawKt;
        }
    } else if (strcasecmp(section, CONFIG_DEBUG_STR) == 0) {
        if (strcasecmp(key, "debug") == 0) {
            return (float)config.debug.debug;
        } else if (strcasecmp(key, "debug_fbw") == 0) {
            return (float)config.debug.debug_fbw;
        } else if (strcasecmp(key, "debug_imu") == 0) {
            return (float)config.debug.debug_imu;
        } else if (strcasecmp(key, "debug_gps") == 0) {
            return (float)config.debug.debug_gps;
        } else if (strcasecmp(key, "debug_wifly") == 0) {
            return (float)config.debug.debug_wifly;
        } else if (strcasecmp(key, "debug_network") == 0) {
            return (float)config.debug.debug_network;
        } else if (strcasecmp(key, "dump_network") == 0) {
            return (float)config.debug.dump_network;
        } else if (strcasecmp(key, "watchdog_timeout_ms") == 0) {
            return (float)config.debug.watchdog_timeout_ms;
        }
    }
    return infinityf();
}

void config_getAllFloats(float values[], size_t numValues) {
    if (values && numValues > 0) {
        size_t index = 0;

        // ConfigGeneral
        values[index++] = (float)config.general.controlMode;
        values[index++] = (float)config.general.switchType;
        values[index++] = (float)config.general.maxCalibrationOffset;
        values[index++] = (float)config.general.servoHz;
        values[index++] = (float)config.general.escHz;
        values[index++] = (float)config.general.apiEnabled;
        values[index++] = (float)config.general.wiflyUsePass;
        values[index++] = (float)config.general.skipCalibration;

        // ConfigControl
        values[index++] = config.control.controlSensitivity;
        values[index++] = config.control.rudderSensitivity;
        values[index++] = config.control.controlDeadband;
        values[index++] = config.control.throttleDetentIdle;
        values[index++] = config.control.throttleDetentMCT;
        values[index++] = config.control.throttleDetentMax;
        values[index++] = (float)config.control.throttleMaxTime;
        values[index++] = infinityf();

        // ConfigLimits
        values[index++] = config.limits.rollLimit;
        values[index++] = config.limits.rollLimitHold;
        values[index++] = config.limits.pitchUpperLimit;
        values[index++] = config.limits.pitchLowerLimit;
        values[index++] = config.limits.maxAilDeflection;
        values[index++] = config.limits.maxElevDeflection;
        values[index++] = config.limits.maxRudDeflection;
        values[index++] = config.limits.maxElevonDeflection;

        // ConfigFlyingWing
        values[index++] = config.flyingWing.elevonMixingGain;
        values[index++] = config.flyingWing.ailMixingBias;
        values[index++] = config.flyingWing.elevMixingBias;
        values[index++] = infinityf();
        values[index++] = infinityf();
        values[index++] = infinityf();
        values[index++] = infinityf();
        values[index++] = infinityf();

        // ConfigPins0
        values[index++] = (float)config.pins0.inputAil;
        values[index++] = (float)config.pins0.servoAil;
        values[index++] = (float)config.pins0.inputElev;
        values[index++] = (float)config.pins0.servoElev;
        values[index++] = (float)config.pins0.inputRud;
        values[index++] = (float)config.pins0.servoRud;
        values[index++] = (float)config.pins0.inputSwitch;
        values[index++] = infinityf();

        // ConfigPins1
        values[index++] = (float)config.pins1.inputThrottle;
        values[index++] = (float)config.pins1.escThrottle;
        values[index++] = (float)config.pins1.servoElevonL;
        values[index++] = (float)config.pins1.servoElevonR;
        values[index++] = (float)config.pins1.reverseRoll;
        values[index++] = (float)config.pins1.reversePitch;
        values[index++] = (float)config.pins1.reverseYaw;
        values[index++] = infinityf();

        // ConfigSensors
        values[index++] = (float)config.sensors.imuModel;
        values[index++] = (float)config.sensors.imuSda;
        values[index++] = (float)config.sensors.imuScl;
        values[index++] = (float)config.sensors.gpsEnabled;
        values[index++] = (float)config.sensors.gpsBaudrate;
        values[index++] = (float)config.sensors.gpsCommandType;
        values[index++] = (float)config.sensors.gpsTx;
        values[index++] = (float)config.sensors.gpsRx;

        // ConfigPID0
        values[index++] = config.pid0.rollTau;
        values[index++] = config.pid0.rollIntegMin;
        values[index++] = config.pid0.rollIntegMax;
        values[index++] = config.pid0.rollKt;
        values[index++] = config.pid0.pitchTau;
        values[index++] = config.pid0.pitchIntegMin;
        values[index++] = config.pid0.pitchIntegMax;
        values[index++] = config.pid0.pitchKt;

        // ConfigPID1
        values[index++] = config.pid1.yawKp;
        values[index++] = config.pid1.yawKi;
        values[index++] = config.pid1.yawKd;
        values[index++] = config.pid1.yawTau;
        values[index++] = config.pid1.yawIntegMin;
        values[index++] = config.pid1.yawIntegMax;
        values[index++] = config.pid1.yawKt;
        values[index++] = infinityf();

        // ConfigDebug
        values[index++] = (float)config.debug.debug;
        values[index++] = (float)config.debug.debug_fbw;
        values[index++] = (float)config.debug.debug_imu;
        values[index++] = (float)config.debug.debug_gps;
        values[index++] = (float)config.debug.debug_wifly;
        values[index++] = (float)config.debug.debug_network;
        values[index++] = (float)config.debug.dump_network;
        values[index++] = (float)config.debug.watchdog_timeout_ms;

        // Fill any remaining values
        while (index < numValues) {
            values[index++] = infinityf();
        }
    }
}

bool config_setFloat(const char *section, const char *key, float value) {
    if (strcasecmp(section, CONFIG_GENERAL_STR) == 0) {
        if (strcasecmp(key, "controlMode") == 0) {
            config.general.controlMode = (ControlMode)value;
            return true;
        } else if (strcasecmp(key, "switchType") == 0) {
            config.general.switchType = (SwitchType)value;
            return true;
        } else if (strcasecmp(key, "maxCalibrationOffset") == 0) {
            config.general.maxCalibrationOffset = (uint8_t)value;
            return true;
        } else if (strcasecmp(key, "servoHz") == 0) {
            config.general.servoHz = (uint)value;
            return true;
        } else if (strcasecmp(key, "escHz") == 0) {
            config.general.escHz = (uint)value;
            return true;
        } else if (strcasecmp(key, "apiEnabled") == 0) {
            config.general.apiEnabled = (bool)value;
            return true;
        } else if (strcasecmp(key, "wiflyUsePass") == 0) {
            config.general.wiflyUsePass = (bool)value;
            return true;
        } else if (strcasecmp(key, "skipCalibration") == 0) {
            config.general.skipCalibration = (bool)value;
            return true;
        }
    } else if (strcasecmp(section, CONFIG_CONTROL_STR) == 0) {
        if (strcasecmp(key, "controlSensitivity") == 0) {
            config.control.controlSensitivity = value;
            return true;
        } else if (strcasecmp(key, "rudderSensitivity") == 0) {
            config.control.rudderSensitivity = value;
            return true;
        } else if (strcasecmp(key, "controlDeadband") == 0) {
            config.control.controlDeadband = value;
            return true;
        } else if (strcasecmp(key, "throttleDetentIdle") == 0) {
            config.control.throttleDetentIdle = value;
            return true;
        } else if (strcasecmp(key, "throttleDetentMCT") == 0) {
            config.control.throttleDetentMCT = value;
            return true;
        } else if (strcasecmp(key, "throttleDetentMax") == 0) {
            config.control.throttleDetentMax = value;
            return true;
        } else if (strcasecmp(key, "throttleMaxTime") == 0) {
            config.control.throttleMaxTime = (uint)value;
            return true;
        }
    } else if (strcasecmp(section, CONFIG_LIMITS_STR) == 0) {
        if (strcasecmp(key, "rollLimit") == 0) {
            config.limits.rollLimit = value;
            return true;
        } else if (strcasecmp(key, "rollLimitHold") == 0) {
            config.limits.rollLimitHold = value;
            return true;
        } else if (strcasecmp(key, "pitchUpperLimit") == 0) {
            config.limits.pitchUpperLimit = value;
            return true;
        } else if (strcasecmp(key, "pitchLowerLimit") == 0) {
            config.limits.pitchLowerLimit = value;
            return true;
        } else if (strcasecmp(key, "maxAilDeflection") == 0) {
            config.limits.maxAilDeflection = value;
            return true;
        } else if (strcasecmp(key, "maxElevDeflection") == 0) {
            config.limits.maxElevDeflection = value;
            return true;
        } else if (strcasecmp(key, "maxRudDeflection") == 0) {
            config.limits.maxRudDeflection = value;
            return true;
        } else if (strcasecmp(key, "maxElevonDeflection") == 0) {
            config.limits.maxElevonDeflection = value;
            return true;
        }
    } else if (strcasecmp(section, CONFIG_FLYING_WING_STR) == 0) {
        if (strcasecmp(key, "elevonMixingGain") == 0) {
            config.flyingWing.elevonMixingGain = value;
            return true;
        } else if (strcasecmp(key, "ailMixingBias") == 0) {
            config.flyingWing.ailMixingBias = value;
            return true;
        } else if (strcasecmp(key, "elevMixingBias") == 0) {
            config.flyingWing.elevMixingBias = value;
            return true;
        }
    } else if (strcasecmp(section, CONFIG_PINS0_STR) == 0) {
        if (strcasecmp(key, "inputAil") == 0) {
            config.pins0.inputAil = (uint)value;
            return true;
        } else if (strcasecmp(key, "servoAil") == 0) {
            config.pins0.servoAil = (uint)value;
            return true;
        } else if (strcasecmp(key, "inputElev") == 0) {
            config.pins0.inputElev = (uint)value;
            return true;
        } else if (strcasecmp(key, "servoElev") == 0) {
            config.pins0.servoElev = (uint)value;
            return true;
        } else if (strcasecmp(key, "inputRud") == 0) {
            config.pins0.inputRud = (uint)value;
            return true;
        } else if (strcasecmp(key, "servoRud") == 0) {
            config.pins0.servoRud = (uint)value;
            return true;
        } else if (strcasecmp(key, "inputSwitch") == 0) {
            config.pins0.inputSwitch = (uint)value;
            return true;
        }
    } else if (strcasecmp(section, CONFIG_PINS1_STR) == 0) {
        if (strcasecmp(key, "inputThrottle") == 0) {
            config.pins1.inputThrottle = (uint)value;
            return true;
        } else if (strcasecmp(key, "escThrottle") == 0) {
            config.pins1.escThrottle = (uint)value;
            return true;
        } else if (strcasecmp(key, "servoElevonL") == 0) {
            config.pins1.servoElevonL = (uint)value;
            return true;
        } else if (strcasecmp(key, "servoElevonR") == 0) {
            config.pins1.servoElevonR = (uint)value;
            return true;
        } else if (strcasecmp(key, "reverseRoll") == 0) {
            config.pins1.reverseRoll = (bool)value;
            return true;
        } else if (strcasecmp(key, "reversePitch") == 0) {
            config.pins1.reversePitch = (bool)value;
            return true;
        } else if (strcasecmp(key, "reverseYaw") == 0) {
            config.pins1.reverseYaw = (bool)value;
            return true;
        }
    } else if (strcasecmp(section, CONFIG_SENSORS_STR) == 0) {
        if (strcasecmp(key, "imuModel") == 0) {
            config.sensors.imuModel = (IMUModel)value;
            return true;
        } else if (strcasecmp(key, "imuSda") == 0) {
            config.sensors.imuSda = (uint)value;
            return true;
        } else if (strcasecmp(key, "imuScl") == 0) {
            config.sensors.imuScl = (uint)value;
            return true;
        } else if (strcasecmp(key, "gpsEnabled") == 0) {
            config.sensors.gpsEnabled = (bool)value;
            return true;
        } else if (strcasecmp(key, "gpsBaudrate") == 0) {
            config.sensors.gpsBaudrate = (uint)value;
            return true;
        } else if (strcasecmp(key, "gpsCommandType") == 0) {
            config.sensors.gpsCommandType = (GPSCommandType)value;
            return true;
        } else if (strcasecmp(key, "gpsTx") == 0) {
            config.sensors.gpsTx = (uint)value;
            return true;
        } else if (strcasecmp(key, "gpsRx") == 0) {
            config.sensors.gpsRx = (uint)value;
            return true;
        }
    } else if (strcasecmp(section, CONFIG_PID0_STR) == 0) {
        if (strcasecmp(key, "rollTau") == 0) {
            config.pid0.rollTau = value;
            return true;
        } else if (strcasecmp(key, "rollIntegMin") == 0) {
            config.pid0.rollIntegMin = value;
            return true;
        } else if (strcasecmp(key, "rollIntegMax") == 0) {
            config.pid0.rollIntegMax = value;
            return true;
        } else if (strcasecmp(key, "rollKt") == 0) {
            config.pid0.rollKt = value;
            return true;
        } else if (strcasecmp(key, "pitchTau") == 0) {
            config.pid0.pitchTau = value;
            return true;
        } else if (strcasecmp(key, "pitchIntegMin") == 0) {
            config.pid0.pitchIntegMin = value;
            return true;
        } else if (strcasecmp(key, "pitchIntegMax") == 0) {
            config.pid0.pitchIntegMax = value;
            return true;
        } else if (strcasecmp(key, "pitchKt") == 0) {
            config.pid0.pitchKt = value;
            return true;
        }
    } else if (strcasecmp(section, CONFIG_PID1_STR) == 0) {
        if (strcasecmp(key, "yawKp") == 0) {
            config.pid1.yawKp = value;
            return true;
        } else if (strcasecmp(key, "yawKi") == 0) {
            config.pid1.yawKi = value;
            return true;
        } else if (strcasecmp(key, "yawKd") == 0) {
            config.pid1.yawKd = value;
            return true;
        } else if (strcasecmp(key, "yawTau") == 0) {
            config.pid1.yawTau = value;
            return true;
        } else if (strcasecmp(key, "yawIntegMin") == 0) {
            config.pid1.yawIntegMin = value;
            return true;
        } else if (strcasecmp(key, "yawIntegMax") == 0) {
            config.pid1.yawIntegMax = value;
            return true;
        } else if (strcasecmp(key, "yawKt") == 0) {
            config.pid1.yawKt = value;
            return true;
        }
    } else if (strcasecmp(section, CONFIG_DEBUG_STR) == 0) {
        if (strcasecmp(key, "debug") == 0) {
            config.debug.debug = (bool)value;
            return true;
        } else if (strcasecmp(key, "debug_fbw") == 0) {
            config.debug.debug_fbw = (bool)value;
            return true;
        } else if (strcasecmp(key, "debug_imu") == 0) {
            config.debug.debug_imu = (bool)value;
            return true;
        } else if (strcasecmp(key, "debug_gps") == 0) {
            config.debug.debug_gps = (bool)value;
            return true;
        } else if (strcasecmp(key, "debug_wifly") == 0) {
            config.debug.debug_wifly = (bool)value;
            return true;
        } else if (strcasecmp(key, "debug_network") == 0) {
            config.debug.debug_network = (bool)value;
            return true;
        } else if (strcasecmp(key, "dump_network") == 0) {
            config.debug.dump_network = (bool)value;
            return true;
        } else if (strcasecmp(key, "watchdog_timeout_ms") == 0) {
            config.debug.watchdog_timeout_ms = (uint32_t)value;
            return true;
        }
    }
    return false;
}

const char *config_getString(const char *section, const char *key) {
    if (strcasecmp(section, CONFIG_WIFLY_STR) == 0) {
        if (strcasecmp(key, "ssid") == 0) {
            return config.wifly.ssid;
        } else if (strcasecmp(key, "pass") == 0) {
            return config.wifly.pass;
        }
    }
    return NULL;
}

void config_getAllStrings(const char *values[], size_t numValues) {
    if (values && numValues > 0) {
        size_t index = 0;

        // ConfigWifly
        values[index++] = config.wifly.ssid;
        values[index++] = config.wifly.pass;

        while (index < numValues) {
            values[index++] = NULL;
        }
    }
}

bool config_setString(const char *section, const char *key, const char *value) {
    if (strcasecmp(section, CONFIG_WIFLY_STR) == 0) {
        if (strcasecmp(key, "ssid") == 0) {
            strncpy(config.wifly.ssid, value, sizeof(config.wifly.ssid));
            config.wifly.ssid[sizeof(config.wifly.ssid) - 1] = '\0';
            return true;
        } else if (strcasecmp(key, "pass") == 0) {
            strncpy(config.wifly.pass, value, sizeof(config.wifly.pass));
            config.wifly.pass[sizeof(config.wifly.pass) - 1] = '\0';
            return true;
        }
    }
    return false;
}
