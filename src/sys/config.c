/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "pico/types.h"

#include "config.h"

ConfigGeneral configGeneral;
ConfigControl configControl;
ConfigLimits configLimits;
ConfigFlyingWing configFlyingWing;
ConfigPins0 configPins0;
ConfigPins1 configPins1;
ConfigSensors configSensors;
ConfigWifly configWifly;
ConfigPID0 configPID0;
ConfigPID1 configPID1;
ConfigDebug configDebug;

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
            configGeneral.controlMode = (ControlMode)flash_readFloat(FLOAT_SECTOR_CONFIG_GENERAL, 0);
            configGeneral.switchType = (SwitchType)flash_readFloat(FLOAT_SECTOR_CONFIG_GENERAL, 1);
            configGeneral.maxCalibrationOffset = (uint8_t)flash_readFloat(FLOAT_SECTOR_CONFIG_GENERAL, 2);
            configGeneral.servoHz = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_GENERAL, 3);
            configGeneral.escHz = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_GENERAL, 4);
            configGeneral.apiEnabled = (bool)flash_readFloat(FLOAT_SECTOR_CONFIG_GENERAL, 5);
            configGeneral.bootWaitMs = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_GENERAL, 6);
            configGeneral.wiflyUsePass = (bool)flash_readFloat(FLOAT_SECTOR_CONFIG_GENERAL, 7);

            // ConfigControl
            configControl.controlSensitivity = flash_readFloat(FLOAT_SECTOR_CONFIG_CONTROL, 0);
            configControl.rudderSensitivity = flash_readFloat(FLOAT_SECTOR_CONFIG_CONTROL, 1);
            configControl.controlDeadband = flash_readFloat(FLOAT_SECTOR_CONFIG_CONTROL, 2);
            configControl.throttleDetentIdle = flash_readFloat(FLOAT_SECTOR_CONFIG_CONTROL, 3);
            configControl.throttleDetentMCT = flash_readFloat(FLOAT_SECTOR_CONFIG_CONTROL, 4);
            configControl.throttleDetentMax = flash_readFloat(FLOAT_SECTOR_CONFIG_CONTROL, 5);
            configControl.throttleMaxTime = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_CONTROL, 6);

            // ConfigLimits
            configLimits.rollLimit = flash_readFloat(FLOAT_SECTOR_CONFIG_LIMITS, 0);
            configLimits.rollLimitHold = flash_readFloat(FLOAT_SECTOR_CONFIG_LIMITS, 1);
            configLimits.pitchUpperLimit = flash_readFloat(FLOAT_SECTOR_CONFIG_LIMITS, 2);
            configLimits.pitchLowerLimit = flash_readFloat(FLOAT_SECTOR_CONFIG_LIMITS, 3);
            configLimits.maxAilDeflection = flash_readFloat(FLOAT_SECTOR_CONFIG_LIMITS, 4);
            configLimits.maxElevDeflection = flash_readFloat(FLOAT_SECTOR_CONFIG_LIMITS, 5);
            configLimits.maxRudDeflection = flash_readFloat(FLOAT_SECTOR_CONFIG_LIMITS, 6);
            configLimits.maxElevonDeflection = flash_readFloat(FLOAT_SECTOR_CONFIG_LIMITS, 7);

            // ConfigFlyingWing
            configFlyingWing.elevonMixingGain = flash_readFloat(FLOAT_SECTOR_CONFIG_FLYINGWING, 0);
            configFlyingWing.ailMixingBias = flash_readFloat(FLOAT_SECTOR_CONFIG_FLYINGWING, 1);
            configFlyingWing.elevMixingBias = flash_readFloat(FLOAT_SECTOR_CONFIG_FLYINGWING, 2);

            // ConfigPins0
            configPins0.inputAil = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_PINS0, 0);
            configPins0.servoAil = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_PINS0, 1);
            configPins0.inputElev = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_PINS0, 2);
            configPins0.servoElev = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_PINS0, 3);
            configPins0.inputRud = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_PINS0, 4);
            configPins0.servoRud = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_PINS0, 5);
            configPins0.inputSwitch = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_PINS0, 6);

            // ConfigPins1
            configPins1.inputThrottle = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_PINS1, 0);
            configPins1.escThrottle = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_PINS1, 1);
            configPins1.servoElevonL = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_PINS1, 2);
            configPins1.servoElevonR = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_PINS1, 3);
            configPins1.reverseRoll = (bool)flash_readFloat(FLOAT_SECTOR_CONFIG_PINS1, 4);
            configPins1.reversePitch = (bool)flash_readFloat(FLOAT_SECTOR_CONFIG_PINS1, 5);
            configPins1.reverseYaw = (bool)flash_readFloat(FLOAT_SECTOR_CONFIG_PINS1, 6);

            // ConfigSensors
            configSensors.imuModel = (IMUModel)flash_readFloat(FLOAT_SECTOR_CONFIG_SENSORS, 0);
            configSensors.imuSda = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_SENSORS, 1);
            configSensors.imuScl = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_SENSORS, 2);
            configSensors.gpsEnabled = (bool)flash_readFloat(FLOAT_SECTOR_CONFIG_SENSORS, 3);
            configSensors.gpsBaudrate = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_SENSORS, 4);
            configSensors.gpsCommandType = (GPSCommandType)flash_readFloat(FLOAT_SECTOR_CONFIG_SENSORS, 5);
            configSensors.gpsTx = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_SENSORS, 6);
            configSensors.gpsRx = (uint)flash_readFloat(FLOAT_SECTOR_CONFIG_SENSORS, 7);

            // ConfigWifly
            const char* ssid = flash_readString(STRING_SECTOR_CONFIG_WIFLY_SSID);
            if (ssid) {
                strncpy(configWifly.ssid, ssid, STRING_SECTOR_SIZE);
            } else {
                return false;
            }
            const char* pass = flash_readString(STRING_SECTOR_CONFIG_WIFLY_PASS);
            if (pass) {
                strncpy(configWifly.pass, pass, STRING_SECTOR_SIZE);
            } else {
                return false;
            }

            // ConfigPID0
            configPID0.rollTau = flash_readFloat(FLOAT_SECTOR_CONFIG_PID0, 0);
            configPID0.rollIntegMin = flash_readFloat(FLOAT_SECTOR_CONFIG_PID0, 1);
            configPID0.rollIntegMax = flash_readFloat(FLOAT_SECTOR_CONFIG_PID0, 2);
            configPID0.rollKt = flash_readFloat(FLOAT_SECTOR_CONFIG_PID0, 3);
            configPID0.pitchTau = flash_readFloat(FLOAT_SECTOR_CONFIG_PID0, 4);
            configPID0.pitchIntegMin = flash_readFloat(FLOAT_SECTOR_CONFIG_PID0, 5);
            configPID0.pitchIntegMax = flash_readFloat(FLOAT_SECTOR_CONFIG_PID0, 6);
            configPID0.pitchKt = flash_readFloat(FLOAT_SECTOR_CONFIG_PID0, 7);

            // ConfigPID1
            configPID1.yawKp = flash_readFloat(FLOAT_SECTOR_CONFIG_PID1, 0);
            configPID1.yawKi = flash_readFloat(FLOAT_SECTOR_CONFIG_PID1, 1);
            configPID1.yawKd = flash_readFloat(FLOAT_SECTOR_CONFIG_PID1, 2);
            configPID1.yawTau = flash_readFloat(FLOAT_SECTOR_CONFIG_PID1, 3);
            configPID1.yawIntegMin = flash_readFloat(FLOAT_SECTOR_CONFIG_PID1, 4);
            configPID1.yawIntegMax = flash_readFloat(FLOAT_SECTOR_CONFIG_PID1, 5);
            configPID1.yawKt = flash_readFloat(FLOAT_SECTOR_CONFIG_PID1, 6);

            // ConfigDebug
            #if defined(LIB_PICO_STDIO_USB) || defined(LIB_PICO_STDIO_UART)
                configDebug.debug = true;
                configDebug.debug_fbw = (bool)flash_readFloat(FLOAT_SECTOR_CONFIG_DEBUG, 1);
                configDebug.debug_imu = (bool)flash_readFloat(FLOAT_SECTOR_CONFIG_DEBUG, 2);
                configDebug.debug_gps = (bool)flash_readFloat(FLOAT_SECTOR_CONFIG_DEBUG, 3);
                configDebug.debug_wifly = (bool)flash_readFloat(FLOAT_SECTOR_CONFIG_DEBUG, 4);
                configDebug.debug_network = (bool)flash_readFloat(FLOAT_SECTOR_CONFIG_DEBUG, 5);
                configDebug.dump_network = (bool)flash_readFloat(FLOAT_SECTOR_CONFIG_DEBUG, 6);
                configDebug.watchdog_timeout_ms = (uint32_t)flash_readFloat(FLOAT_SECTOR_CONFIG_DEBUG, 7);
            #else
                // All debugging is automatically disabled when stdio is not enabled
                configDebug.debug = false;
                configDebug.debug_fbw = false;
                configDebug.debug_imu = false;
                configDebug.debug_gps = false;
                configDebug.debug_wifly = false;
                configDebug.debug_network = false;
                configDebug.dump_network = false;
            #endif
            break;
        case DEFAULT_VALUES:
            // Very similar except load from DEFault macros instead of flash (and thus no validation)
            // ConfigGeneral
            configGeneral.controlMode = CONTROL_MODE_DEF;
            configGeneral.switchType = SWITCH_TYPE_DEF;
            configGeneral.maxCalibrationOffset = MAX_CALIBRATION_OFFSET_DEF;
            configGeneral.servoHz = SERVO_HZ_DEF;
            configGeneral.escHz = ESC_HZ_DEF;
            configGeneral.apiEnabled = API_ENABLED_DEF;
            configGeneral.bootWaitMs = BOOT_WAIT_MS_DEF;
            configGeneral.wiflyUsePass = WIFLY_USE_PASS_DEF;

            // ConfigControl
            configControl.controlSensitivity = CONTROL_SENSITIVITY_DEF;
            configControl.rudderSensitivity = RUDDER_SENSITIVITY_DEF;
            configControl.controlDeadband = CONTROL_DEADBAND_DEF;
            configControl.throttleDetentIdle = THROTTLE_DETENT_IDLE_DEF;
            configControl.throttleDetentMCT = THROTTLE_DETENT_MCT_DEF;
            configControl.throttleDetentMax = THROTTLE_DETENT_MAX_DEF;
            configControl.throttleMaxTime = THROTTLE_MAX_TIME_DEF;

            // ConfigLimits
            configLimits.rollLimit = ROLL_LIMIT_DEF;
            configLimits.rollLimitHold = ROLL_LIMIT_HOLD_DEF;
            configLimits.pitchUpperLimit = PITCH_UPPER_LIMIT_DEF;
            configLimits.pitchLowerLimit = PITCH_LOWER_LIMIT_DEF;
            configLimits.maxAilDeflection = MAX_AIL_DEFLECTION_DEF;
            configLimits.maxElevDeflection = MAX_ELEV_DEFLECTION_DEF;
            configLimits.maxRudDeflection = MAX_RUD_DEFLECTION_DEF;
            configLimits.maxElevonDeflection = MAX_ELEVON_DEFLECTION_DEF;

            // ConfigFlyingWing
            configFlyingWing.elevonMixingGain = ELEVON_MIXING_GAIN_DEF;
            configFlyingWing.ailMixingBias = AIL_MIXING_BIAS_DEF;
            configFlyingWing.elevMixingBias = ELEV_MIXING_BIAS_DEF;

            // ConfigPins0
            configPins0.inputAil = INPUT_AIL_DEF;
            configPins0.servoAil = SERVO_AIL_DEF;
            configPins0.inputElev = INPUT_ELEV_DEF;
            configPins0.servoElev = SERVO_ELEV_DEF;
            configPins0.inputRud = INPUT_RUD_DEF;
            configPins0.servoRud = SERVO_RUD_DEF;
            configPins0.inputSwitch = INPUT_SWITCH_DEF;

            // ConfigPins1
            configPins1.inputThrottle = INPUT_THROTTLE_DEF;
            configPins1.escThrottle = ESC_THROTTLE_DEF;
            configPins1.servoElevonL = SERVO_ELEVON_L_DEF;
            configPins1.servoElevonR = SERVO_ELEVON_R_DEF;
            configPins1.reverseRoll = REVERSE_ROLL_DEF;
            configPins1.reversePitch = REVERSE_PITCH_DEF;
            configPins1.reverseYaw = REVERSE_YAW_DEF;

            // ConfigSensors
            configSensors.imuModel = IMU_MODEL_DEF;
            configSensors.imuSda = IMU_SDA_DEF;
            configSensors.imuScl = IMU_SCL_DEF;
            configSensors.gpsEnabled = GPS_ENABLED_DEF;
            configSensors.gpsBaudrate = GPS_BAUDRATE_DEF;
            configSensors.gpsCommandType = GPS_COMMAND_TYPE_DEF;
            configSensors.gpsTx = GPS_TX_DEF;
            configSensors.gpsRx = GPS_RX_DEF;

            // ConfigWifly
            strncpy(configWifly.ssid, WIFLY_SSID_DEF, STRING_SECTOR_SIZE);
            strncpy(configWifly.pass, WIFLY_PASS_DEF, STRING_SECTOR_SIZE);

            // ConfigPID0
            configPID0.rollTau = ROLL_TAU_DEF;
            configPID0.rollIntegMin = ROLL_INTEG_MIN_DEF;
            configPID0.rollIntegMax = ROLL_INTEG_MAX_DEF;
            configPID0.rollKt = ROLL_KT_DEF;
            configPID0.pitchTau = PITCH_TAU_DEF;
            configPID0.pitchIntegMin = PITCH_INTEG_MIN_DEF;
            configPID0.pitchIntegMax = PITCH_INTEG_MAX_DEF;
            configPID0.pitchKt = PITCH_KT_DEF;

            // ConfigPID1
            configPID1.yawKp = YAW_KP_DEF;
            configPID1.yawKi = YAW_KI_DEF;
            configPID1.yawKd = YAW_KD_DEF;
            configPID1.yawTau = YAW_TAU_DEF;
            configPID1.yawIntegMin = YAW_INTEG_MIN_DEF;
            configPID1.yawIntegMax = YAW_INTEG_MAX_DEF;
            configPID1.yawKt = YAW_KT_DEF;

            // ConfigDebug
            #if defined(LIB_PICO_STDIO_USB) || defined(LIB_PICO_STDIO_UART)
                configDebug.debug = true;
                configDebug.debug_fbw = DEBUG_FBW_DEF;
                configDebug.debug_imu = DEBUG_IMU_DEF;
                configDebug.debug_gps = DEBUG_GPS_DEF;
                configDebug.debug_wifly = DEBUG_WIFLY_DEF;
                configDebug.debug_network = DEBUG_NETWORK_DEF;
                configDebug.dump_network = DUMP_NETWORK_DEF;
            #else
                configDebug.debug = false;
                configDebug.debug_fbw = false;
                configDebug.debug_imu = false;
                configDebug.debug_gps = false;
                configDebug.debug_wifly = false;
                configDebug.debug_network = false;
                configDebug.dump_network = false;
            #endif
            configDebug.watchdog_timeout_ms = WATCHDOG_TIMEOUT_MS_DEF;
            break;
    }
    return true;
}

// TODO: config validation before saving (replaces what is currently in validator.h)

bool config_save() {
    // ConfigGeneral
    float general[FLOAT_SECTOR_SIZE] = {
        configGeneral.controlMode,
        configGeneral.switchType,
        configGeneral.maxCalibrationOffset,
        configGeneral.servoHz,
        configGeneral.escHz,
        configGeneral.apiEnabled,
        configGeneral.bootWaitMs,
        configGeneral.wiflyUsePass
    }; // C already casts all elements in the array to a float so there's no need to explicitly cast them
    flash_writeFloat(FLOAT_SECTOR_CONFIG_GENERAL, general);

    // ConfigControl
    float control[FLOAT_SECTOR_SIZE] = {
        configControl.controlSensitivity,
        configControl.rudderSensitivity,
        configControl.controlDeadband,
        configControl.throttleDetentIdle,
        configControl.throttleDetentMCT,
        configControl.throttleDetentMax,
        configControl.throttleMaxTime
    };
    flash_writeFloat(FLOAT_SECTOR_CONFIG_CONTROL, control);

    // ConfigLimits
    float limits[FLOAT_SECTOR_SIZE] = {
        configLimits.rollLimit,
        configLimits.rollLimitHold,
        configLimits.pitchUpperLimit,
        configLimits.pitchLowerLimit,
        configLimits.maxAilDeflection,
        configLimits.maxElevDeflection,
        configLimits.maxRudDeflection,
        configLimits.maxElevonDeflection
    };
    flash_writeFloat(FLOAT_SECTOR_CONFIG_LIMITS, limits);

    // ConfigFlyingWing
    float flyingWing[FLOAT_SECTOR_SIZE] = {
        configFlyingWing.elevonMixingGain,
        configFlyingWing.ailMixingBias,
        configFlyingWing.elevMixingBias
    };
    flash_writeFloat(FLOAT_SECTOR_CONFIG_FLYINGWING, flyingWing);

    // ConfigPins0
    float pins0[FLOAT_SECTOR_SIZE] = {
        configPins0.inputAil,
        configPins0.servoAil,
        configPins0.inputElev,
        configPins0.servoElev,
        configPins0.inputRud,
        configPins0.servoRud,
        configPins0.inputSwitch
    };
    flash_writeFloat(FLOAT_SECTOR_CONFIG_PINS0, pins0);

    // ConfigPins1
    float pins1[FLOAT_SECTOR_SIZE] = {
        configPins1.inputThrottle,
        configPins1.escThrottle,
        configPins1.servoElevonL,
        configPins1.servoElevonR,
        configPins1.reverseRoll,
        configPins1.reversePitch,
        configPins1.reverseYaw
    };
    flash_writeFloat(FLOAT_SECTOR_CONFIG_PINS1, pins1);

    // ConfigSensors
    float sensors[FLOAT_SECTOR_SIZE] = {
        configSensors.imuModel,
        configSensors.imuSda,
        configSensors.imuScl,
        configSensors.gpsEnabled,
        configSensors.gpsBaudrate,
        configSensors.gpsCommandType,
        configSensors.gpsTx,
        configSensors.gpsRx
    };
    flash_writeFloat(FLOAT_SECTOR_CONFIG_SENSORS, sensors);

    // ConfigWifly -- uses StringsSectors not floats
    flash_writeString(STRING_SECTOR_CONFIG_WIFLY_SSID, configWifly.ssid);
    flash_writeString(STRING_SECTOR_CONFIG_WIFLY_PASS, configWifly.pass);

    // ConfigPID0
    float rollPitchPID[FLOAT_SECTOR_SIZE] = {
        configPID0.rollTau,
        configPID0.rollIntegMin,
        configPID0.rollIntegMax,
        configPID0.rollKt,
        configPID0.pitchTau,
        configPID0.pitchIntegMin,
        configPID0.pitchIntegMax,
        configPID0.pitchKt
    };
    flash_writeFloat(FLOAT_SECTOR_CONFIG_PID0, rollPitchPID);

    // ConfigPID1
    float yawPID[FLOAT_SECTOR_SIZE] = {
        configPID1.yawKp,
        configPID1.yawKi,
        configPID1.yawKd,
        configPID1.yawTau,
        configPID1.yawIntegMin,
        configPID1.yawIntegMax,
        configPID1.yawKt
    };
    flash_writeFloat(FLOAT_SECTOR_CONFIG_PID1, yawPID);

    // ConfigDebug
    float debug[FLOAT_SECTOR_SIZE] = {
        configDebug.debug,
        configDebug.debug_fbw,
        configDebug.debug_imu,
        configDebug.debug_gps,
        configDebug.debug_wifly,
        configDebug.debug_network,
        configDebug.dump_network,
        configDebug.watchdog_timeout_ms
    };
    flash_writeFloat(FLOAT_SECTOR_CONFIG_DEBUG, debug);
    return true;
}

