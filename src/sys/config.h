#ifndef __CONFIG_H
#define __CONFIG_H

#include "../io/baro.h"
#include "../io/flash.h"
#include "../io/gps.h"
#include "../io/imu.h"
#include "../io/pwm.h"
#include "../wifly/wifly.h"

#include "switch.h"

/* The amount of time (in ms) to wait for any possible serial connections to be established before booting.
This option is compiled in, as configuration is not yet loaded when this value is needed. */
#define BOOT_WAIT_MS 1000

typedef enum ConfigSource {
    FROM_FLASH,
    DEFAULT_VALUES
} ConfigSource;

typedef enum ConfigSectionType {
    SECTION_TYPE_NONE,
    SECTION_TYPE_FLOAT,
    SECTION_TYPE_STRING
} ConfigSectionType;

#define CONFIG_GENERAL_STR "General"
typedef struct ConfigGeneral {
    ControlMode controlMode; // Also stores athrEnabled
    SwitchType switchType;
    uint8_t maxCalibrationOffset;
    uint servoHz;
    uint escHz;
    bool apiEnabled;
    WiflyEnableStatus wiflyStatus;
    bool skipCalibration;
} ConfigGeneral;

#define CONTROL_MODE_DEF CTRLMODE_3AXIS_ATHR
#define SWITCH_TYPE_DEF SWITCH_TYPE_3_POS
#define MAX_CALIBRATION_OFFSET_DEF 20
#define SERVO_HZ_DEF 50
#define ESC_HZ_DEF 50
#define API_ENABLED_DEF true
#define WIFLY_STATUS_DEF WIFLY_ENABLED_PASS
#define SKIP_CALIBRATION_DEF false

#define CONFIG_CONTROL_STR "Control"
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

#define CONTROL_SENSITIVITY_DEF 0.00075
#define RUDDER_SENSITIVITY_DEF 1.5
#define CONTROL_DEADBAND_DEF 2
#define THROTTLE_DETENT_IDLE_DEF 10
#define THROTTLE_DETENT_MCT_DEF 75
#define THROTTLE_DETENT_MAX_DEF 90
#define THROTTLE_MAX_TIME_DEF 10

#define CONFIG_LIMITS_STR "Limits"
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

#define ROLL_LIMIT_DEF 33
#define ROLL_LIMIT_HOLD_DEF 67
#define PITCH_UPPER_LIMIT_DEF 30
#define PITCH_LOWER_LIMIT_DEF -15
#define MAX_AIL_DEFLECTION_DEF 25
#define MAX_ELEV_DEFLECTION_DEF 15
#define MAX_RUD_DEFLECTION_DEF 20
#define MAX_ELEVON_DEFLECTION_DEF 20

#define CONFIG_FLYING_WING_STR "FlyingWing"
typedef struct ConfigFlyingWing {
    float elevonMixingGain;
    float ailMixingBias;
    float elevMixingBias;
    // 4, 5, 6, 7, 8
} ConfigFlyingWing;

#define ELEVON_MIXING_GAIN_DEF 0.5
#define AIL_MIXING_BIAS_DEF 1
#define ELEV_MIXING_BIAS_DEF 1

#define CONFIG_PINS0_STR "Pins0"
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

#define INPUT_AIL_DEF 1
#define SERVO_AIL_DEF 2
#define INPUT_ELEV_DEF 3
#define SERVO_ELEV_DEF 4
#define INPUT_RUD_DEF 5
#define SERVO_RUD_DEF 6
#define INPUT_SWITCH_DEF 9

#define CONFIG_PINS1_STR "Pins1"
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

#define INPUT_THROTTLE_DEF 7
#define ESC_THROTTLE_DEF 8
#define SERVO_ELEVON_L_DEF 2
#define SERVO_ELEVON_R_DEF 4
#define REVERSE_ROLL_DEF false
#define REVERSE_PITCH_DEF false
#define REVERSE_YAW_DEF false

#define CONFIG_SENSORS_STR "Sensors"
typedef struct ConfigSensors {
    IMUModel imuModel;
    uint imuSda;
    uint imuScl;
    BaroModel baroModel;
    uint gpsBaudrate;
    GPSCommandType gpsCommandType; // Also stores gpsEnabled
    uint gpsTx;
    uint gpsRx;
} ConfigSensors;

#define IMU_MODEL_DEF IMU_MODEL_BNO055
#define IMU_SDA_DEF 16
#define IMU_SCL_DEF 17
#define BARO_MODEL_DEF BARO_MODEL_NONE
#define GPS_BAUDRATE_DEF 9600
#define GPS_COMMAND_TYPE_DEF GPS_COMMAND_TYPE_PMTK
#define GPS_TX_DEF 21
#define GPS_RX_DEF 20

#define CONFIG_PID0_STR "PID0"
typedef struct ConfigPID0 {
    float rollTau;
    float rollIntegMin;
    float rollIntegMax;
    float rollKt;
    float pitchTau;
    float pitchIntegMin;
    float pitchIntegMax;
    float pitchKt;
} ConfigPID0;

#define ROLL_TAU_DEF 0.001
#define ROLL_INTEG_MIN_DEF -50.0
#define ROLL_INTEG_MAX_DEF 50.0
#define ROLL_KT_DEF 0.01
#define PITCH_TAU_DEF 0.001
#define PITCH_INTEG_MIN_DEF -50.0
#define PITCH_INTEG_MAX_DEF 50.0
#define PITCH_KT_DEF 0.01

#define CONFIG_PID1_STR "PID1"
typedef struct ConfigPID1 {
    float yawKp;
    float yawKi;
    float yawKd;
    float yawTau;
    float yawIntegMin;
    float yawIntegMax;
    float yawKt;
    // 8
} ConfigPID1;

#define YAW_KP_DEF 1.0
#define YAW_KI_DEF 0.0025
#define YAW_KD_DEF 0.001
#define YAW_TAU_DEF 0.001
#define YAW_INTEG_MIN_DEF -50.0
#define YAW_INTEG_MAX_DEF 50.0
#define YAW_KT_DEF 0.01

#define CONFIG_DEBUG_STR "Debug"
typedef struct ConfigDebug {
    bool debug;
    bool debug_fbw;
    bool debug_imu;
    bool debug_gps;
    bool debug_wifly;
    bool debug_network;
    bool dump_network;
    uint32_t watchdog_timeout_ms; 
} ConfigDebug;

#define DEBUG_FBW_DEF true
#define DEBUG_IMU_DEF false
#define DEBUG_GPS_DEF false
#define DEBUG_WIFLY_DEF false
#define DEBUG_NETWORK_DEF false
#define DUMP_NETWORK_DEF false
#define WATCHDOG_TIMEOUT_MS_DEF 4000

#define CONFIG_WIFLY_STR "Wifly"
typedef struct ConfigWifly {
    char ssid[STRING_SECTOR_SIZE];
    char pass[STRING_SECTOR_SIZE];
} ConfigWifly;

#define WIFLY_SSID_DEF "pico-fbw"
#define WIFLY_PASS_DEF "wiflyfbw"

typedef enum ConfigSection {
    CONFIG_GENERAL,
    CONFIG_CONTROL,
    CONFIG_LIMITS,
    CONFIG_FLYING_WING,
    CONFIG_PINS0,
    CONFIG_PINS1,
    CONFIG_SENSORS,
    CONFIG_PID0,
    CONFIG_PID1,
    CONFIG_DEBUG,
    CONFIG_WIFLY
} ConfigSection;

typedef struct Config {
    ConfigGeneral general;
    ConfigControl control;
    ConfigLimits limits;
    ConfigFlyingWing flyingWing;
    ConfigPins0 pins0;
    ConfigPins1 pins1;
    ConfigSensors sensors;
    ConfigPID0 pid0;
    ConfigPID1 pid1;
    ConfigDebug debug;
    ConfigWifly wifly;
} Config;

extern Config config;

#define NUM_CONFIG_SECTIONS 11
#define NUM_FLOAT_CONFIG_SECTIONS 10
#define NUM_STRING_CONFIG_SECTIONS 1
#define NUM_FLOAT_VALUES_PER_SECTION 8 // Based on FLOAT_SECTOR_SIZE
#define NUM_FLOAT_CONFIG_VALUES (NUM_FLOAT_CONFIG_SECTIONS * NUM_FLOAT_VALUES_PER_SECTION)
#define NUM_STRING_VALUES_PER_SECTION 2 // Cause there's only one string section lol
#define NUM_STRING_CONFIG_VALUES (NUM_STRING_CONFIG_SECTIONS * NUM_STRING_VALUES_PER_SECTION)

/**
 * Loads the config from flash into RAM (or loads default values into RAM).
 * @param source source of the config data (whether to load from flash or defaults)
 * @return true if loading was successful, false otherwise (likely due to uninitialized/corrupt data).
*/
bool config_load(ConfigSource source);

/**
 * Saves the config from RAM into flash.
 * @param validate whether to validate the config before saving (true) or not (false)
 * @return true if saving was successful, false if one or more configuration values are invalid (did not pass validation).
*/
bool config_save(bool validate);

/**
 * @param section section of the config to get the type of
 * @return the type of the config section (float or string)
*/
ConfigSectionType config_getSectionType(const char *section);

/**
 * @param section section of the config to get the string representation of
 * @return the string representation (name) of the config section
*/
const char *config_sectionToString(ConfigSection section);

/**
 * Gets a value from a float section of the config.
 * @param section section of the config to get the value from
 * @param key key of the value to get
 * @return the value of the config, or inf if the value does not exist
*/
float config_getFloat(const char* section, const char* key);

/**
 * Gets all float values from the config (up until max size of numValues).
 * @param values array to store the values in
 * @param numValues number of values to store
 * @note numValues should be at least NUM_FLOAT_CONFIG_VALUES to avoid overflow
*/
void config_getAllFloats(float values[], size_t numValues);

/**
 * Sets a value in a float section of the config.
 * @param section name of the section to set the value in
 * @param key name of the key to set the value in
 * @param value value to set
 * @return true if the value was set, false if the section or key does not exist
*/
bool config_setFloat(const char *section, const char *key, float value);

/**
 * Gets a value from a string section of the config.
 * @param section name of the section of the config to get the value from
 * @param key name of the key to set the value in
 * @return the value of the config, or NULL if the value does not exist
*/
const char *config_getString(const char *section, const char *key);

/**
 * Gets all string values from the config (up until max size of numValues).
 * @param values array to store the values in
 * @param numValues number of values to store
 * @note numValues should be at least NUM_STRING_CONFIG_VALUES to avoid overflow
*/
void config_getAllStrings(const char *values[], size_t numValues);

/**
 * Gets a value from a string section of the config.
 * @param section section of the config to get the value from
 * @param key name of the key to set the value in
 * @param value value to set
 * @return true if the value was set, false if the section or key does not exist
*/
bool config_setString(const char *section, const char *key, const char *value);

#endif // __CONFIG_H