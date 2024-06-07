#pragma once

#include <stdbool.h>
#include "platform/stdio.h"

/* The amount of time (in ms) to wait for any possible serial connections to be established before booting.
This option is compiled in, as the config is not yet loaded when this value is needed. */
#define BOOT_WAIT_MS 1000

// -- Config struct details --

#define CONFIG_SECTION_SIZE 32
#define CONFIG_STR_SIZE 128
#define NUM_FLOAT_CONFIG_SECTIONS 5
#define NUM_STRING_CONFIG_SECTIONS 1
#define NUM_CONFIG_SECTIONS (NUM_FLOAT_CONFIG_SECTIONS + NUM_STRING_CONFIG_SECTIONS)
#define CONFIG_END_MAGIC (-30.54245f) // Denotes the end of a config section

// -- Config section indices and definition --

typedef enum ConfigGeneral {
    GENERAL_CONTROL_MODE,
    GENERAL_SWITCH_TYPE,
    GENERAL_MAX_CALIBRATION_OFFSET,
    GENERAL_SERVO_HZ,
    GENERAL_ESC_HZ,
    GENERAL_API_ENABLED,
    GENERAL_WIFI_ENABLED,
    GENERAL_LAUNCHASSIST_ENABLED,
    GENERAL_SKIP_CALIBRATION,
} ConfigGeneral;

typedef enum ConfigControl {
    // Control handling preferences
    CONTROL_MAX_ROLL_RATE,
    CONTROL_MAX_PITCH_RATE,
    CONTROL_RUDDER_SENSITIVITY,
    CONTROL_DEADBAND,
    // Throttle detent/autothrottle configuration
    CONTROL_THROTTLE_MAX_TIME,
    CONTROL_THROTTLE_COOLDOWN_TIME,
    CONTROL_THROTTLE_SENSITIVITY,
    // Drop bay detent settings
    CONTROL_DROP_DETENT_CLOSED,
    CONTROL_DROP_DETENT_OPEN,
    // Control limits
    CONTROL_ROLL_LIMIT,
    CONTROL_ROLL_LIMIT_HOLD,
    CONTROL_PITCH_LOWER_LIMIT,
    CONTROL_PITCH_UPPER_LIMIT,
    // Physical control surface limits
    CONTROL_MAX_AIL_DEFLECTION,
    CONTROL_MAX_ELE_DEFLECTION,
    CONTROL_MAX_RUD_DEFLECTION,
    // Flying wing configuration
    CONTROL_MAX_ELEVON_DEFLECTION,
    CONTROL_ELEVON_MIXING_GAIN,
    CONTROL_AIL_MIXING_BIAS,
    CONTROL_ELEV_MIXING_BIAS,
} ConfigControl;

typedef enum ConfigPins {
    // Control IO pins
    PINS_INPUT_AIL,
    PINS_SERVO_AIL,
    PINS_INPUT_ELE,
    PINS_SERVO_ELE,
    PINS_INPUT_RUD,
    PINS_SERVO_RUD,
    PINS_INPUT_THROTTLE,
    PINS_ESC_THROTTLE,
    PINS_INPUT_SWITCH,
    PINS_SERVO_BAY,
    // Sensor communications pins
    PINS_AAHRS_SDA,
    PINS_AAHRS_SCL,
    PINS_GPS_TX,
    PINS_GPS_RX,
    // Servo reverse flags
    PINS_REVERSE_ROLL,
    PINS_REVERSE_PITCH,
    PINS_REVERSE_YAW,
} ConfigPins;
#define S_PIN_MIN PINS_INPUT_AIL
#define S_PIN_MAX PINS_GPS_RX

typedef enum ConfigSensors {
    SENSORS_IMU_MODEL,
    SENSORS_BARO_MODEL,
    SENSORS_AAHRS_BUS_FREQ,
    SENSORS_GPS_COMMAND_TYPE,
    SENSORS_GPS_BAUDRATE,
} ConfigSensors;

typedef enum ConfigSystem {
    SYSTEM_USE_DISPLAY,
    SYSTEM_PRINT_FBW,
    SYSTEM_PRINT_AAHRS,
    SYSTEM_PRINT_AIRCRAFT,
    SYSTEM_PRINT_GPS,
    SYSTEM_PRINT_NETWORK,
} ConfigSystem;

typedef struct ConfigWifi {
    char ssid[CONFIG_STR_SIZE];
    char pass[CONFIG_STR_SIZE];
} ConfigWifi;

typedef struct Config {
    f32 general[CONFIG_SECTION_SIZE];
#define CONFIG_GENERAL_STR "General"
    f32 control[CONFIG_SECTION_SIZE];
#define CONFIG_CONTROL_STR "Control"
    f32 pins[CONFIG_SECTION_SIZE];
#define CONFIG_PINS_STR "Pins"
    f32 sensors[CONFIG_SECTION_SIZE];
#define CONFIG_SENSORS_STR "Sensors"
    f32 system[CONFIG_SECTION_SIZE];
#define CONFIG_SYSTEM_STR "System"
    ConfigWifi wifi;
#define CONFIG_WIFI_STR "WiFi"
} Config;

// -- Calibration struct indices and definition --

typedef enum CalibrationPWM {
    PWM_CALIBRATED,
    PWM_MODE,
    PWM_OFFSET_AIL,
    PWM_OFFSET_ELE,
    PWM_OFFSET_RUD,
    PWM_OFFSET_SW,
    PWM_OFFSET_THR,
} CalibrationPWM;

typedef enum CalibrationESC {
    ESC_CALIBRATED,
    ESC_DETENT_IDLE,
    ESC_DETENT_MCT,
    ESC_DETENT_MAX,
} CalibrationESC;

typedef enum CalibrationAAHRS {
    AAHRS_CALIBRATED,
    AAHRS_IMU_MODEL,
    AAHRS_BARO_MODEL,
} CalibrationAAHRS;

typedef enum CalibrationPID {
    PID_TUNED,
    // Roll PID parameters
    PID_ROLL_KP,
    PID_ROLL_KI,
    PID_ROLL_KD,
    PID_ROLL_TAU,
    PID_ROLL_INTEGMIN,
    PID_ROLL_INTEGMAX,
    // Pitch PID parameters
    PID_PITCH_KP,
    PID_PITCH_KI,
    PID_PITCH_KD,
    PID_PITCH_TAU,
    PID_PITCH_INTEGMIN,
    PID_PITCH_INTEGMAX,
    // Yaw PID parameters
    PID_YAW_KP,
    PID_YAW_KI,
    PID_YAW_KD,
    PID_YAW_TAU,
    PID_YAW_INTEGMIN,
    PID_YAW_INTEGMAX,
    // Throttle PID parameters
    PID_THROTTLE_KP,
    PID_THROTTLE_KI,
    PID_THROTTLE_KD,
    PID_THROTTLE_TAU,
    PID_THROTTLE_INTEGMIN,
    PID_THROTTLE_INTEGMAX,
} CalibrationPID;

typedef struct Calibration {
    f32 pwm[CONFIG_SECTION_SIZE];
#define CONFIG_PWM_STR "PWM"
    f32 esc[CONFIG_SECTION_SIZE];
#define CONFIG_ESC_STR "ESC"
    f32 aahrs[CONFIG_SECTION_SIZE];
#define CONFIG_AAHRS_STR "AAHRS"
    f32 pid[CONFIG_SECTION_SIZE];
#define CONFIG_PID_STR "PID"
} Calibration;

// -- Config section type and enum conversion functions (for lookup functions) --

typedef enum ConfigSectionType {
    SECTION_TYPE_NONE,
    SECTION_TYPE_FLOAT,
    SECTION_TYPE_STRING,
} ConfigSectionType;

typedef enum ConfigSection {
    CONFIG_GENERAL,
    CONFIG_CONTROL,
    CONFIG_PINS,
    CONFIG_SENSORS,
    CONFIG_SYSTEM,
    CONFIG_WIFI,
} ConfigSection;

// -- Config functions --

/**
 * Loads the config from flash memory into the config struct.
 * If the config is invalid/nonexistant, it will be reset to default values.
 */
void config_load();

/**
 * Saves the current config to flash memory.
 */
void config_save();

/**
 * Resets the config to default values.
 * @note This function simply erases the config file from flash memory, as such,
 * a reboot is required to load the "new" default config.
 */
void config_reset();

/**
 * @return Whether the current config is valid.
 */
bool config_validate();

/**
 * Gets a value from the config based on its string representation.
 * @param section the name of the section to look in
 * @param key the name of the key to look up
 * @param value the pointer to the value to store the result in
 * @return type of the value stored
 * @note Value will store either an `f32` or `const char*` depending on the type of section and should be cast accordingly.
 */
ConfigSectionType config_get(const char *section, const char *key, void **value);

/**
 * Sets a value in the config based on its string representation.
 * @param section the name of the section to look in
 * @param key the name of the key to look up
 * @param value the string represenation to the value to store, will be parsed into other formats as needed
 * @return Whether the value was successfully set.
 */
bool config_set(const char *section, const char *key, const char *value);

/**
 * Gets a string representation of a config section based on its (enum) index, as well as its type.
 * @param section the (enum) index of the section
 * @param str pointer to a string to store the result in
 * @return the type of the section
 */
ConfigSectionType config_to_string(ConfigSection section, const char **str);

extern Config config;
extern Calibration calibration;
