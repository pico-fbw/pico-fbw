#pragma once

// <sys/config.h> is a standard C include, hence why this file is called configuration.h!

#include "platform/types.h"

/* The amount of time (in ms) to wait for any possible serial connections to be established before booting.
This option is compiled in, as the config is not yet loaded when this value is needed. */
#define BOOT_WAIT_MS 1000

// -- Config struct details --

#define CONFIG_STR_SIZE 128
#define CONFIG_SECTION_COUNT 6

// -- Config section definitions --

typedef struct ConfigGeneralSection {
    f32 controlMode;
    f32 switchType;
    f32 maxCalibrationOffset;
    f32 servoHz, escHz;
    f32 apiEnabled;
    f32 wifiEnabled;
    f32 launchAssistEnabled;
    f32 autoTuneEnabled;
    f32 skipCalibration;
} ConfigGeneralSection;

typedef struct ConfigControlSection {
    f32 maxRollRate, maxPitchRate;
    f32 expo;
    f32 rudderSensitivity;
    f32 controlDeadband;
    f32 throttleMaxTime, throttleCooldownTime, throttleSensitivity;
    f32 dropDetentClosed, dropDetentOpen;
    f32 rollLimit, rollLimitHold;
    f32 pitchLowerLimit, pitchUpperLimit;
    f32 maxAilDeflection, maxEleDeflection, maxRudDeflection;
    f32 maxElevonDeflection, elevonMixingGain;
    f32 ailMixingBias, elevMixingBias;
} ConfigControlSection;

typedef struct ConfigPinsSection {
    f32 inputAil, servoAil;
    f32 inputEle, servoEle;
    f32 inputRud, servoRud;
    f32 inputThrottle, escThrottle;
    f32 inputSwitch;
    f32 servoBay;
    f32 i2cSda, i2cScl;
    f32 spiClk, spiMosi, spiMiso;
    f32 spiCs0, spiCs1, spiCs2;
    f32 gpsTx, gpsRx;
    f32 reverseRoll, reversePitch, reverseYaw;
} ConfigPinsSection;

typedef struct ConfigSensorsSection {
    f32 busType;
    f32 i2cBusFreq, spiBusFreq;
    f32 gpsCommandType, gpsBaudrate;
} ConfigSensorsSection;

typedef struct ConfigSystemSection {
    char ssid[CONFIG_STR_SIZE];
    char pass[CONFIG_STR_SIZE];
    f32 printsys;
    f32 printIMU;
    f32 printAircraft;
    f32 printGPS;
    f32 printNetwork;
} ConfigSystemSection;

typedef struct ConfigWebUISection {
    f32 altSamples;
    f32 defaultSpeed;
    f32 dropSecs;
    char pilotName[CONFIG_STR_SIZE];
    f32 defaultMap;
    char lastMapPosition[CONFIG_STR_SIZE];
    char lastMapZoom[CONFIG_STR_SIZE];
    f32 setupComplete;
} ConfigWebUISection;

typedef struct Config {
    ConfigGeneralSection general;
#define CONFIG_GENERAL_STR "General"
    ConfigControlSection control;
#define CONFIG_CONTROL_STR "Control"
    ConfigPinsSection pins;
#define CONFIG_PINS_STR "Pins"
    ConfigSensorsSection sensors;
#define CONFIG_SENSORS_STR "Sensors"
    ConfigSystemSection system;
#define CONFIG_SYSTEM_STR "System"
    ConfigWebUISection webui;
#define CONFIG_WEBUI_STR "WebUI"
    const char *version; // Not a config section; identifier for this config struct's saved version
} Config;

// -- Calibration section definitions --

typedef struct CalibrationPWMSection {
    f32 calibrated;
    f32 mode;
    f32 offsetAil;
    f32 offsetEle;
    f32 offsetRud;
    f32 offsetSw;
    f32 offsetThr;
} CalibrationPWMSection;

typedef struct CalibrationESCSection {
    f32 calibrated;
    f32 detentIdle;
    f32 detentMct;
    f32 detentMax;
} CalibrationESCSection;

typedef struct CalibrationIMUSection {
    f32 calibrated;
    f32 gyroBiasX;
    f32 gyroBiasY;
    f32 gyroBiasZ;
    f32 accelOffsetX;
    f32 accelOffsetY;
    f32 accelOffsetZ;
    f32 axisMapRoll;
    f32 axisMapPitch;
    f32 axisMapYaw;
    f32 axisSignRoll;
    f32 axisSignPitch;
    f32 axisSignYaw;
} CalibrationIMUSection;

typedef struct CalibrationPIDSection {
    f32 tuned;
    f32 rollKp;
    f32 rollKi;
    f32 rollKd;
    f32 rollDb;
    f32 pitchKp;
    f32 pitchKi;
    f32 pitchKd;
    f32 pitchDb;
    f32 yawKp;
    f32 yawKi;
    f32 yawKd;
    f32 yawDb;
    f32 throttleKp;
    f32 throttleKi;
    f32 throttleKd;
    f32 tau;
} CalibrationPIDSection;

typedef struct Calibration {
    CalibrationPWMSection pwm;
#define CONFIG_PWM_STR "PWM"
    CalibrationESCSection esc;
#define CONFIG_ESC_STR "ESC"
    CalibrationIMUSection imu;
#define CONFIG_IMU_STR "IMU"
    CalibrationPIDSection pid;
#define CONFIG_PID_STR "PID"
    const char *version;
} Calibration;

// -- Config section types (for lookup functions) --

typedef enum ConfigSectionType {
    SECTION_TYPE_NONE,
    SECTION_TYPE_NUMBER,
    SECTION_TYPE_STRING,
} ConfigSectionType;

typedef enum ConfigSection {
    CONFIG_GENERAL,
    CONFIG_CONTROL,
    CONFIG_PINS,
    CONFIG_SENSORS,
    CONFIG_SYSTEM,
    CONFIG_WEBUI,
} ConfigSection;

typedef struct ConfigEntry {
    const char *key;
    ConfigSectionType type;
    void *value;
} ConfigEntry;

typedef struct ConfigSectionInfo {
    const char *name;
    ConfigSectionType type;
    const ConfigEntry *entries;
    size_t entryCount;
} ConfigSectionInfo;

// -- Config functions --

typedef enum ConfigSetResult {
    CONFIG_SET_OK,
    CONFIG_SET_DOES_NOT_EXIST, // Given section/key does not exist
    CONFIG_SET_INVALID, // Validation failed and config was not set, run config_validate() to get the error message
} ConfigSetResult;

/**
 * Loads the config from flash memory into the config struct.
 * If the config is invalid/nonexistant, it will be reset to default values.
 */
void config_load();

/**
 * Saves the current config to flash memory.
 * @note This function does no validation, use `config_validate` to first check that the config is valid.
 */
void config_save();

/**
 * Resets the config to default values.
 * @note This function simply erases the config file from flash memory, as such,
 * a reboot is required to load the "new" default config.
 */
void config_reset();

/**
 * @param error a buffer to store a possible error message in
 * @param error_size the size of the error buffer
 * @return whether the current config is valid
 * @note The buffer should be at least 128 bytes long
 */
bool config_validate(char *error, size_t error_size);

/**
 * Gets the section metadata for a config section.
 * @param section the section to inspect
 * @return the metadata for the requested section, or NULL if it does not exist
 */
const ConfigSectionInfo *config_section_info(ConfigSection section);

/**
 * Finds a config entry by section/key name.
 * @param section the section name to look in
 * @param key the key name to look up
 * @return the matching config entry, or NULL if it does not exist
 */
const ConfigEntry *config_find_entry(const char *section, const char *key);

/**
 * Gets a value from the config based on its string representation.
 * @param section the name of the section to look in
 * @param key the name of the key to look up
 * @param value the pointer to the value to store the result in
 * @return type of the value stored
 * @note Value will store either an `f32` or `const char*` depending on the type of section and should be cast
 * accordingly.
 */
ConfigSectionType config_get(const char *section, const char *key, void **value);

/**
 * Sets a value in the config based on its string representation.
 * @param section the name of the section to look in
 * @param key the name of the key to look up
 * @param value the string represenation to the value to store, will be parsed into other formats as needed
 * @return whether the value was successfully set.
 */
ConfigSetResult config_set(const char *section, const char *key, const char *value);

/**
 * Backs up the current config to be restored later.
 */
void config_backup();

/**
 * Restores the backed up config.
 */
void config_restore();

extern Config config;
extern Calibration calibration;
