#ifndef __CONFIG_H
#define __CONFIG_H

#include "../io/flash.h"
#include "../io/gps.h"
#include "../io/imu.h"
#include "../io/pwm.h"
#include "switch.h"

typedef struct ConfigGeneral {
    ControlMode controlMode; // Also stores athrEnabled
    SwitchType switchType;
    uint8_t maxCalibrationOffset;
    uint servoHz;
    uint escHz;
    bool apiEnabled;
    uint bootWaitMs;
    bool wiflyUsePass;
} ConfigGeneral;

typedef struct ConfigControl {
    float controlSensitivity;
    float rudderSensitivity;
    float controlDeadband;
    float throttleDetentIdle;
    float throttleDetentMCT;
    float throttleDetentMax;
    uint throttleMaxTime;
    // 8
} ConfigControl;

typedef struct ConfigLimits {
    float rollLimit;
    float rollLimitHold;
    float pitchUpperLimit;
    float pitchLowerLimit;
    float maxAilDeflection;
    float maxElevDeflection;
    float maxRudDeflection;
    float maxElevonDeflection;
} ConfigLimits;

typedef struct ConfigFlyingWing {
    float elevonMixingGain;
    float ailMixingBias;
    float elevMixingBias;
    // 4, 5, 6, 7, 8
} ConfigFlyingWing;

typedef struct ConfigPins0 {
    uint inputAil;
    uint servoAil;
    uint inputElev;
    uint servoElev;
    uint inputRud;
    uint servoRud;
    uint inputSwitch;
    // 8
} ConfigPins0;

typedef struct ConfigPins1 {
    uint inputThrottle;
    uint escThrottle;
    uint servoElevonL;
    uint servoElevonR;
    bool reverseRoll;
    bool reversePitch;
    bool reverseYaw;
    // 8
} ConfigPins1;

typedef struct ConfigSensors {
    IMUModel imuModel;
    uint imuSda;
    uint imuScl;
    bool gpsEnabled;
    uint gpsBaudrate;
    GPSCommandType gpsCommandType;
    uint gpsTx;
    uint gpsRx;
} ConfigSensors;

typedef struct ConfigWifly {
    char ssid[STRING_SECTOR_SIZE];
    char pass[STRING_SECTOR_SIZE];
} ConfigWifly;

typedef struct ConfigRollPitchPID {
    float rollTau;
    float rollIntegMin;
    float rollIntegMax;
    float rollKt;
    float pitchTau;
    float pitchIntegMin;
    float pitchIntegMax;
    float pitchKt;
} ConfigRollPitchPID;

typedef struct ConfigYawPID {
    float yawKp;
    float yawKi;
    float yawKd;
    float yawTau;
    float yawIntegMin;
    float yawIntegMax;
    float yawKt;
    // 8
} ConfigYawPID;

typedef struct ConfigDebug {
    bool debug; // TODO: set this on bootup based on if USB/UART enabled
    bool debug_fbw;
    bool debug_imu;
    bool debug_gps;
    bool debug_wifly;
    bool debug_network;
    bool dump_wifly;
    bool dump_network;
} ConfigDebug;

extern ConfigGeneral configGeneral;
extern ConfigControl configControl;
extern ConfigLimits configLimits;
extern ConfigFlyingWing configFlyingWing;
extern ConfigPins0 configPins0;
extern ConfigPins1 configPins1;
extern ConfigSensors configSensors;
extern ConfigWifly configWifly;
extern ConfigRollPitchPID configRollPitchPID;
extern ConfigYawPID configYawPID;
extern ConfigDebug configDebug;

/**
 * Loads the config from flash into RAM.
 * @return true if loading was successful, false otherwise (likely due to uninitialized/corrupt data).
*/
bool config_load();

/**
 * Saves the config from RAM into flash.
 * @return true if saving was successful, false if one or more configuration values are invalid (did not pass validation).
*/
bool config_save();

#endif // __CONFIG_H