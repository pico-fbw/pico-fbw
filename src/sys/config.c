/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "pico/types.h"

#include "../io/flash.h"

#include "config.h"

ConfigGeneral configGeneral;
ConfigControl configControl;
ConfigLimits configLimits;
ConfigFlyingWing configFlyingWing;
ConfigPins0 configPins0;
ConfigPins1 configPins1;
ConfigSensors configSensors;
ConfigWifly configWifly;
ConfigRollPitchPID configRollPitchPID;
ConfigYawPID configYawPID;
ConfigDebug configDebug;

bool config_load() {
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
        strcpy(configWifly.ssid, ssid);
    } else {
        return false;
    }
    const char* pass = flash_readString(STRING_SECTOR_CONFIG_WIFLY_PASS);
    if (pass) {
        strcpy(configWifly.pass, pass);
    } else {
        return false;
    }

    // ConfigRollPitchPID
    configRollPitchPID.rollTau = flash_readFloat(FLOAT_SECTOR_CONFIG_ROLLPITCHPID, 0);
    configRollPitchPID.rollIntegMin = flash_readFloat(FLOAT_SECTOR_CONFIG_ROLLPITCHPID, 1);
    configRollPitchPID.rollIntegMax = flash_readFloat(FLOAT_SECTOR_CONFIG_ROLLPITCHPID, 2);
    configRollPitchPID.rollKt = flash_readFloat(FLOAT_SECTOR_CONFIG_ROLLPITCHPID, 3);
    configRollPitchPID.pitchTau = flash_readFloat(FLOAT_SECTOR_CONFIG_ROLLPITCHPID, 4);
    configRollPitchPID.pitchIntegMin = flash_readFloat(FLOAT_SECTOR_CONFIG_ROLLPITCHPID, 5);
    configRollPitchPID.pitchIntegMax = flash_readFloat(FLOAT_SECTOR_CONFIG_ROLLPITCHPID, 6);
    configRollPitchPID.pitchKt = flash_readFloat(FLOAT_SECTOR_CONFIG_ROLLPITCHPID, 7);

    // ConfigYawPID
    configYawPID.yawKp = flash_readFloat(FLOAT_SECTOR_CONFIG_YAWPID, 0);
    configYawPID.yawKi = flash_readFloat(FLOAT_SECTOR_CONFIG_YAWPID, 1);
    configYawPID.yawKd = flash_readFloat(FLOAT_SECTOR_CONFIG_YAWPID, 2);
    configYawPID.yawTau = flash_readFloat(FLOAT_SECTOR_CONFIG_YAWPID, 3);
    configYawPID.yawIntegMin = flash_readFloat(FLOAT_SECTOR_CONFIG_YAWPID, 4);
    configYawPID.yawIntegMax = flash_readFloat(FLOAT_SECTOR_CONFIG_YAWPID, 5);
    configYawPID.yawKt = flash_readFloat(FLOAT_SECTOR_CONFIG_YAWPID, 6);

    // ConfigDebug
    #if defined(LIB_PICO_STDIO_USB) || defined(LIB_PICO_STDIO_UART)
        configDebug.debug = true;
        configDebug.debug_fbw = (bool)flash_readFloat(FLOAT_SECTOR_CONFIG_DEBUG, 1);
        configDebug.debug_imu = (bool)flash_readFloat(FLOAT_SECTOR_CONFIG_DEBUG, 2);
        configDebug.debug_gps = (bool)flash_readFloat(FLOAT_SECTOR_CONFIG_DEBUG, 3);
        configDebug.debug_wifly = (bool)flash_readFloat(FLOAT_SECTOR_CONFIG_DEBUG, 4);
        configDebug.debug_network = (bool)flash_readFloat(FLOAT_SECTOR_CONFIG_DEBUG, 5);
        configDebug.dump_wifly = (bool)flash_readFloat(FLOAT_SECTOR_CONFIG_DEBUG, 6);
        configDebug.dump_network = (bool)flash_readFloat(FLOAT_SECTOR_CONFIG_DEBUG, 7);
    #else
        // All debugging is automatically disabled when stdio is not enabled
        configDebug.debug = false;
        configDebug.debug_fbw = false;
        configDebug.debug_imu = false;
        configDebug.debug_gps = false;
        configDebug.debug_wifly = false;
        configDebug.debug_network = false;
        configDebug.dump_wifly = false;
        configDebug.dump_network = false;
    #endif

    // Validate the configuration
    for (FloatSector s = FLOAT_SECTOR_CONFIG_GENERAL; s <= FLOAT_SECTOR_CONFIG_YAWPID; s++) {
        for (uint v = 0; v < FLOAT_SECTOR_SIZE; v++) {
            if (!isfinite(flash_readFloat(s, v))) return false; // Not initialized?
        }
    }
    return true; // Passed
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

    // ConfigRollPitchPID
    float rollPitchPID[FLOAT_SECTOR_SIZE] = {
        configRollPitchPID.rollTau,
        configRollPitchPID.rollIntegMin,
        configRollPitchPID.rollIntegMax,
        configRollPitchPID.rollKt,
        configRollPitchPID.pitchTau,
        configRollPitchPID.pitchIntegMin,
        configRollPitchPID.pitchIntegMax,
        configRollPitchPID.pitchKt
    };
    flash_writeFloat(FLOAT_SECTOR_CONFIG_ROLLPITCHPID, rollPitchPID);

    // ConfigYawPID
    float yawPID[FLOAT_SECTOR_SIZE] = {
        configYawPID.yawKp,
        configYawPID.yawKi,
        configYawPID.yawKd,
        configYawPID.yawTau,
        configYawPID.yawIntegMin,
        configYawPID.yawIntegMax,
        configYawPID.yawKt
    };
    flash_writeFloat(FLOAT_SECTOR_CONFIG_YAWPID, yawPID);

    // ConfigDebug
    float debug[FLOAT_SECTOR_SIZE] = {
        configDebug.debug,
        configDebug.debug_fbw,
        configDebug.debug_imu,
        configDebug.debug_gps,
        configDebug.debug_wifly,
        configDebug.debug_network,
        configDebug.dump_wifly,
        configDebug.dump_network
    };
    flash_writeFloat(FLOAT_SECTOR_CONFIG_DEBUG, debug);
    return true;
}

