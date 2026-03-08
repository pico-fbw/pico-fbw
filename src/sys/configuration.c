/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

// TODO: redo config system to...not be so confusing
// restructure with littlefs in mind this time
// maybe just key-values
// TODO: also add a section for webui settings (see www/src/helpers/settings.ts)

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform/defs.h"
#include "platform/flash.h"
#include "platform/types.h"
#include "platform/wifi.h"

#include "ctrl/switch.h"
#include "io/gps.h"
#include "io/imu.h"
#include "io/receiver.h"
#include "sys/print.h"
#include "sys/runtime.h"
#include "sys/version.h"

#include "configuration.h"

#define FILE_CONFIG "config.dat"
#define FILE_CALIBRATION "calibration.dat"

// clang-format off

static Config backedUpConfig;

// Default configuration values

Config config = {
    .general = {
        CTRLMODE_2AXIS_ATHR, SWITCH_TYPE_3_POS, 20, 50, 50, true,
        // Wi-Fi should be enabled by default, but only if the platform supports it
        #if PLATFORM_SUPPORTS_WIFI
            WIFI_ENABLED_PASS,
        #else
            WIFI_DISABLED,
        #endif
        false,
        true,
        // Host platforms don't have the necessary hardware to pass calibration, so skip it by default
        #if FBW_PLATFORM_HOST
            true,
        #else
            false,
        #endif
        CONFIG_END_MAGIC,
    },
    .control = {
        25, 15, 1.5f, 2.f, // Control handling preferences
        10, 30, 0.015f, // Autothrottle configuration
        180, 0, // Drop bay detent settings
        33, 67, -15, 30, // Control limits
        25, 15, 20, // Physical control surface limits
        20, 0.5f, 1, 1, // Flying wing configuration
        CONFIG_END_MAGIC,
    },
    .pins = {
        // Control IO pins
        DEFAULT_PIN_INPUT_AIL, DEFAULT_PIN_SERVO_AIL, DEFAULT_PIN_INPUT_ELE, DEFAULT_PIN_SERVO_ELE,
        DEFAULT_PIN_INPUT_RUD, DEFAULT_PIN_SERVO_RUD, DEFAULT_PIN_INPUT_THR, DEFAULT_PIN_ESC_THR,
        DEFAULT_PIN_INPUT_SWITCH, DEFAULT_PIN_SERVO_BAY,
        // Sensor communications pins
        DEFAULT_PIN_I2C_SDA, DEFAULT_PIN_I2C_SCL, DEFAULT_PIN_GPS_TX, DEFAULT_PIN_GPS_RX,
        // Servo reverse flags
        false, false, false,
        CONFIG_END_MAGIC,
    },
    .sensors = {
        400, // I2C configuration
        GPS_COMMAND_TYPE_PMTK, 9600, // GPS configuration
        CONFIG_END_MAGIC,
    },
    .system = {
        true, false, false, false, false, // Default print settings, also found in PrintDefs below
        CONFIG_END_MAGIC,
    },
    .wifi = {
        .ssid = "pico-fbw",
        .pass = "picodashfbw",
    },
    .version = CONFIG_VERSION,
};

Calibration calibration = {
    .pwm = {
        false,
        CTRLMODE_2AXIS_ATHR,
        0, 0, 0, 0, 0 // Default PWM offsets
    },
    .esc = {
        false,
        10, 75, 90, // Default throttle detents
    },
    .imu = {
        false,
        0, 0, 0, // Default gyro bias
        0, 0, 0, // Default accel offset
        0, 1, 2, // Default axis map (identity)
        1, 1, 1, // Default axis signs
    },
    .pid = {
        false,
        // TODO: tune throttle pid, keep checking others (they're mostly tuned already)
        1.5f, 0.4f, 8.f, // Default roll PID parameters
        1.5f, 0.4f, 8.f, // Default pitch PID parameters
        2.f, 0.05f, 3.f, // Default yaw PID parameters
        25.f, 0.11f, 40.f, // Default autothrottle PID parameters
        // TODO: find working tau
        1.f, // Default PID tau
    },
    .version = CALIBRATION_VERSION,
};

PrintDefs shouldPrint = {
    // Default print settings
    true, false, false, false, false
};

// clang-format on

/**
 * Loads the contents of a file into a struct, creating the file based on the curent struct state if it doesn't exist.
 * @param file the path to the file to load
 * @param strct pointer to the struct to load the file into (would be named struct but that's a reserved keyword)
 * @param size the size of the struct
 * @return true if the file was loaded successfully
 */
static bool load_file_to_struct(const char *file, void *strct, size_t size) {
    lfs_file_t f;
    if (lfs_file_open(&lfs, &f, file, LFS_O_RDONLY) != LFS_ERR_OK) {
        // File doesn't exist, create it and write default values which are present in the struct definition
        if (lfs_file_open(&lfs, &f, file, LFS_O_RDWR | LFS_O_CREAT) != LFS_ERR_OK) {
            return false;
        }
        if (lfs_file_write(&lfs, &f, strct, size) != (lfs_ssize_t)size) {
            lfs_file_close(&lfs, &f);
            return false;
        }
        lfs_file_sync(&lfs, &f);
    }
    // Overwrite the struct with the file contents
    if (lfs_file_read(&lfs, &f, strct, size) != (lfs_ssize_t)size) {
        lfs_file_close(&lfs, &f);
        return false;
    }
    return lfs_file_close(&lfs, &f) == LFS_ERR_OK;
}

/**
 * Saves the contents of a struct to a file. The file must exist.
 * @param file the path to the file to save
 * @param strct pointer to the struct to save to the file (would be named struct but that's a reserved keyword)
 * @param size the size of the struct
 */
static bool save_struct_to_file(const char *file, void *strct, size_t size) {
    lfs_file_t f;
    if (lfs_file_open(&lfs, &f, file, LFS_O_WRONLY | LFS_O_TRUNC) != LFS_ERR_OK) {
        return false;
    }
    if (lfs_file_write(&lfs, &f, strct, size) != (lfs_ssize_t)size) {
        lfs_file_close(&lfs, &f);
        return false;
    }
    return lfs_file_close(&lfs, &f) == LFS_ERR_OK;
}

void config_load() {
    load_file_to_struct(FILE_CONFIG, &config, sizeof(config));
    load_file_to_struct(FILE_CALIBRATION, &calibration, sizeof(calibration));
    // Load print settings and set debug flag
    shouldPrint.fbw = config.system[SYSTEM_PRINT_FBW];
    shouldPrint.imu = config.system[SYSTEM_PRINT_IMU];
    shouldPrint.aircraft = config.system[SYSTEM_PRINT_AIRCRAFT];
    shouldPrint.gps = config.system[SYSTEM_PRINT_GPS];
    shouldPrint.network = config.system[SYSTEM_PRINT_NETWORK];
}

void config_save() {
    save_struct_to_file(FILE_CONFIG, &config, sizeof(config));
    save_struct_to_file(FILE_CALIBRATION, &calibration, sizeof(calibration));
}

void config_reset() {
    lfs_remove(&lfs, FILE_CONFIG);
    lfs_remove(&lfs, FILE_CALIBRATION);
}

// I'm so sorry (this is a C moment)

#define GENERAL_KEY_LIST                                                                                               \
    X("controlMode", GENERAL_CONTROL_MODE)                                                                             \
    X("switchType", GENERAL_SWITCH_TYPE)                                                                               \
    X("maxCalibrationOffset", GENERAL_MAX_CALIBRATION_OFFSET)                                                          \
    X("servoHz", GENERAL_SERVO_HZ)                                                                                     \
    X("escHz", GENERAL_ESC_HZ)                                                                                         \
    X("apiEnabled", GENERAL_API_ENABLED)                                                                               \
    X("wifiEnabled", GENERAL_WIFI_ENABLED)                                                                             \
    X("launchAssistEnabled", GENERAL_LAUNCHASSIST_ENABLED)                                                             \
    X("autoTuneEnabled", GENERAL_AUTOTUNE_ENABLED)                                                                     \
    X("skipCalibration", GENERAL_SKIP_CALIBRATION)

#define CONTROL_KEY_LIST                                                                                               \
    X("maxRollRate", CONTROL_MAX_ROLL_RATE)                                                                            \
    X("maxPitchRate", CONTROL_MAX_PITCH_RATE)                                                                          \
    X("rudderSensitivity", CONTROL_RUDDER_SENSITIVITY)                                                                 \
    X("controlDeadband", CONTROL_DEADBAND)                                                                             \
    X("throttleMaxTime", CONTROL_THROTTLE_MAX_TIME)                                                                    \
    X("throttleCooldownTime", CONTROL_THROTTLE_COOLDOWN_TIME)                                                          \
    X("throttleSensitivity", CONTROL_THROTTLE_SENSITIVITY)                                                             \
    X("dropDetentClosed", CONTROL_DROP_DETENT_CLOSED)                                                                  \
    X("dropDetentOpen", CONTROL_DROP_DETENT_OPEN)                                                                      \
    X("rollLimit", CONTROL_ROLL_LIMIT)                                                                                 \
    X("rollLimitHold", CONTROL_ROLL_LIMIT_HOLD)                                                                        \
    X("pitchLowerLimit", CONTROL_PITCH_LOWER_LIMIT)                                                                    \
    X("pitchUpperLimit", CONTROL_PITCH_UPPER_LIMIT)                                                                    \
    X("maxAilDeflection", CONTROL_MAX_AIL_DEFLECTION)                                                                  \
    X("maxEleDeflection", CONTROL_MAX_ELE_DEFLECTION)                                                                  \
    X("maxRudDeflection", CONTROL_MAX_RUD_DEFLECTION)                                                                  \
    X("maxElevonDeflection", CONTROL_MAX_ELEVON_DEFLECTION)                                                            \
    X("elevonMixingGain", CONTROL_ELEVON_MIXING_GAIN)                                                                  \
    X("ailMixingBias", CONTROL_AIL_MIXING_BIAS)                                                                        \
    X("elevMixingBias", CONTROL_ELEV_MIXING_BIAS)

#define PINS_KEY_LIST                                                                                                  \
    X("inputAil", PINS_INPUT_AIL)                                                                                      \
    X("servoAil", PINS_SERVO_AIL)                                                                                      \
    X("inputEle", PINS_INPUT_ELE)                                                                                      \
    X("servoEle", PINS_SERVO_ELE)                                                                                      \
    X("inputRud", PINS_INPUT_RUD)                                                                                      \
    X("servoRud", PINS_SERVO_RUD)                                                                                      \
    X("inputThrottle", PINS_INPUT_THROTTLE)                                                                            \
    X("escThrottle", PINS_ESC_THROTTLE)                                                                                \
    X("inputSwitch", PINS_INPUT_SWITCH)                                                                                \
    X("servoBay", PINS_SERVO_BAY)                                                                                      \
    X("i2cSda", PINS_I2C_SDA)                                                                                          \
    X("i2cScl", PINS_I2C_SCL)                                                                                          \
    X("gpsTx", PINS_GPS_TX)                                                                                            \
    X("gpsRx", PINS_GPS_RX)                                                                                            \
    X("reverseRoll", PINS_REVERSE_ROLL)                                                                                \
    X("reversePitch", PINS_REVERSE_PITCH)                                                                              \
    X("reverseYaw", PINS_REVERSE_YAW)

#define SENSORS_KEY_LIST                                                                                               \
    X("i2cBusFreq", SENSORS_I2C_BUS_FREQ)                                                                              \
    X("gpsCommandType", SENSORS_GPS_COMMAND_TYPE)                                                                      \
    X("gpsBaudrate", SENSORS_GPS_BAUDRATE)

#define SYSTEM_KEY_LIST                                                                                                \
    X("printsys", SYSTEM_PRINT_FBW)                                                                                    \
    X("printIMU", SYSTEM_PRINT_IMU)                                                                                    \
    X("printAircraft", SYSTEM_PRINT_AIRCRAFT)                                                                          \
    X("printGPS", SYSTEM_PRINT_GPS)                                                                                    \
    X("printNetwork", SYSTEM_PRINT_NETWORK)

#define WIFI_KEY_LIST                                                                                                  \
    X("ssid", ssid)                                                                                                    \
    X("pass", pass)

static void get_from_general(const char *key, f32 **value) {
#define X(KEY, INDEX)                                                                                                  \
    if (strcasecmp(key, KEY) == 0) {                                                                                   \
        *value = &config.general[INDEX];                                                                               \
        return;                                                                                                        \
    }
    GENERAL_KEY_LIST
#undef X
    *value = NULL;
}

static bool set_to_general(const char *key, f32 value) {
#define X(KEY, INDEX)                                                                                                  \
    if (strcasecmp(key, KEY) == 0) {                                                                                   \
        config.general[INDEX] = value;                                                                                 \
        return true;                                                                                                   \
    }
    GENERAL_KEY_LIST
#undef X
    return false;
}

static void get_from_control(const char *key, f32 **value) {
#define X(KEY, INDEX)                                                                                                  \
    if (strcasecmp(key, KEY) == 0) {                                                                                   \
        *value = &config.control[INDEX];                                                                               \
        return;                                                                                                        \
    }
    CONTROL_KEY_LIST
#undef X
    *value = NULL;
}

static bool set_to_control(const char *key, f32 value) {
#define X(KEY, INDEX)                                                                                                  \
    if (strcasecmp(key, KEY) == 0) {                                                                                   \
        config.control[INDEX] = value;                                                                                 \
        return true;                                                                                                   \
    }
    CONTROL_KEY_LIST
#undef X
    return false;
}

static void get_from_pins(const char *key, f32 **value) {
#define X(KEY, INDEX)                                                                                                  \
    if (strcasecmp(key, KEY) == 0) {                                                                                   \
        *value = &config.pins[INDEX];                                                                                  \
        return;                                                                                                        \
    }
    PINS_KEY_LIST
#undef X
    *value = NULL;
}

static bool set_to_pins(const char *key, f32 value) {
#define X(KEY, INDEX)                                                                                                  \
    if (strcasecmp(key, KEY) == 0) {                                                                                   \
        config.pins[INDEX] = value;                                                                                    \
        return true;                                                                                                   \
    }
    PINS_KEY_LIST
#undef X
    return false;
}

static void get_from_sensors(const char *key, f32 **value) {
#define X(KEY, INDEX)                                                                                                  \
    if (strcasecmp(key, KEY) == 0) {                                                                                   \
        *value = &config.sensors[INDEX];                                                                               \
        return;                                                                                                        \
    }
    SENSORS_KEY_LIST
#undef X
    *value = NULL;
}

static bool set_to_sensors(const char *key, f32 value) {
#define X(KEY, INDEX)                                                                                                  \
    if (strcasecmp(key, KEY) == 0) {                                                                                   \
        config.sensors[INDEX] = value;                                                                                 \
        return true;                                                                                                   \
    }
    SENSORS_KEY_LIST
#undef X
    return false;
}

static void get_from_system(const char *key, f32 **value) {
#define X(KEY, INDEX)                                                                                                  \
    if (strcasecmp(key, KEY) == 0) {                                                                                   \
        *value = &config.system[INDEX];                                                                                \
        return;                                                                                                        \
    }
    SYSTEM_KEY_LIST
#undef X
    *value = NULL;
}

static bool set_to_system(const char *key, f32 value) {
#define X(KEY, INDEX)                                                                                                  \
    if (strcasecmp(key, KEY) == 0) {                                                                                   \
        config.system[INDEX] = value;                                                                                  \
        return true;                                                                                                   \
    }
    SYSTEM_KEY_LIST
#undef X
    return false;
}

static void get_from_wifi(const char *key, char **value) {
#define X(KEY, MEMBER)                                                                                                 \
    if (strcasecmp(key, KEY) == 0) {                                                                                   \
        *value = config.wifi.MEMBER;                                                                                   \
        return;                                                                                                        \
    }
    WIFI_KEY_LIST
#undef X
    *value = NULL;
}

static bool set_to_wifi(const char *key, const char *value) {
#define X(KEY, MEMBER)                                                                                                 \
    if (strcasecmp(key, KEY) == 0) {                                                                                   \
        strcpy(config.wifi.MEMBER, value);                                                                             \
        return true;                                                                                                   \
    }
    WIFI_KEY_LIST
#undef X
    return false;
}

bool config_validate(char *error, size_t error_size) {
    // Enum limit validation
    // Don't cast to the enum type, it will break the comparison
    if (config.general[GENERAL_CONTROL_MODE] < CTRLMODE_MIN || config.general[GENERAL_CONTROL_MODE] > CTRLMODE_MAX) {
        snprintf(error, error_size, "Control mode must be between %d and %d.", CTRLMODE_MIN, CTRLMODE_MAX);
        return false;
    }
    if (config.general[GENERAL_SWITCH_TYPE] < SWITCH_TYPE_MIN ||
        config.general[GENERAL_SWITCH_TYPE] > SWITCH_TYPE_MAX) {
        snprintf(error, error_size, "Switch type must be between %d and %d.", SWITCH_TYPE_MIN, SWITCH_TYPE_MAX);
        return false;
    }
    if (config.general[GENERAL_WIFI_ENABLED] < WIFI_ENABLED_MIN ||
        config.general[GENERAL_WIFI_ENABLED] > WIFI_ENABLED_MAX) {
        snprintf(error, error_size, "Wi-Fi enable status must be between %d and %d.", WIFI_ENABLED_MIN,
                 WIFI_ENABLED_MAX);
        return false;
    }
    if (config.sensors[SENSORS_GPS_COMMAND_TYPE] < GPS_COMMAND_TYPE_MIN ||
        config.sensors[SENSORS_GPS_COMMAND_TYPE] > GPS_COMMAND_TYPE_MAX) {
        snprintf(error, error_size, "GPS command type must be between %d and %d.", GPS_COMMAND_TYPE_MIN,
                 GPS_COMMAND_TYPE_MAX);
        return false;
    }
    // Unique pin validation
    i32 prevPin = -1;
    for (u32 i = S_PIN_MIN; i <= S_PIN_NOT_FLAG_MAX; i++) {
        i16 pin = config.pins[i];
        if (pin < 0) {
            // < 0 is invalid and means the pin is unused; don't validate it
            continue;
        }
        switch ((ControlMode)config.general[GENERAL_CONTROL_MODE]) {
            case CTRLMODE_3AXIS_ATHR:
                if (pin == prevPin) {
                    goto invalid;
                }
                break;
            case CTRLMODE_3AXIS:
                // Skip pins that aren't utilized in this mode
                if (i == PINS_INPUT_THROTTLE || i == PINS_ESC_THROTTLE) {
                    break;
                }
                if (pin == prevPin) {
                    goto invalid;
                }
                break;
            case CTRLMODE_2AXIS_ATHR:
            case CTRLMODE_FLYINGWING_ATHR:
                if (i == PINS_INPUT_RUD) {
                    break;
                }
                if (pin == prevPin) {
                    goto invalid;
                }
                break;
            case CTRLMODE_2AXIS:
            case CTRLMODE_FLYINGWING:
                if (i == PINS_INPUT_RUD || i == PINS_INPUT_THROTTLE || i == PINS_ESC_THROTTLE) {
                    break;
                }
                if (pin == prevPin) {
                    goto invalid;
                }
                break;
            invalid:
                snprintf(error, error_size, "A pin may only be used once.");
                return false;
        }
        prevPin = pin;
    }
    // Limit validation
    if (config.control[CONTROL_ROLL_LIMIT] > 72 || config.control[CONTROL_ROLL_LIMIT] < 0) {
        snprintf(error, error_size, "Roll limit must be between 0 and 72 degrees.");
        return false;
    }
    if (config.control[CONTROL_ROLL_LIMIT_HOLD] > 72 || config.control[CONTROL_ROLL_LIMIT_HOLD] < 0) {
        snprintf(error, error_size, "Roll limit hold must be between 0 and 72 degrees.");
        return false;
    }
    if (config.control[CONTROL_PITCH_UPPER_LIMIT] > 35 || config.control[CONTROL_PITCH_UPPER_LIMIT] < 0) {
        snprintf(error, error_size, "Upper pitch limit must be between 0 and 35 degrees.");
        return false;
    }
    if (config.control[CONTROL_PITCH_LOWER_LIMIT] < -20 || config.control[CONTROL_PITCH_LOWER_LIMIT] > 0) {
        snprintf(error, error_size, "Lower pitch limit must be between -20 and 0 degrees.");
        return false;
    }
    // Throttle configuration validation
    if (config.control[CONTROL_THROTTLE_SENSITIVITY] < 0.f || config.control[CONTROL_THROTTLE_SENSITIVITY] > 1.0f) {
        snprintf(error, error_size, "Throttle sensitivity must be between 0.0 and 1.0.");
        return false;
    }
    // Drop (servo position) validation
    if (config.control[CONTROL_DROP_DETENT_CLOSED] < 0 || config.control[CONTROL_DROP_DETENT_OPEN] > 180) {
        snprintf(error, error_size, "Drop detent (closed) must be between 0 and 180 degrees.");
        return false;
    }
    if (config.control[CONTROL_DROP_DETENT_OPEN] < 0 || config.control[CONTROL_DROP_DETENT_OPEN] > 180) {
        snprintf(error, error_size, "Drop detent (open) must be between 0 and 180 degrees.");
        return false;
    }
    // Control limit validation
    switch ((ControlMode)config.general[GENERAL_CONTROL_MODE]) {
        case CTRLMODE_3AXIS_ATHR:
        case CTRLMODE_3AXIS:
        case CTRLMODE_2AXIS_ATHR:
        case CTRLMODE_2AXIS:
            if (config.control[CONTROL_MAX_AIL_DEFLECTION] > 90 || config.control[CONTROL_MAX_AIL_DEFLECTION] < 0) {
                snprintf(error, error_size, "Max aileron deflection must be between 0 and 90 degrees.");
                return false;
            }
            if (config.control[CONTROL_MAX_ELE_DEFLECTION] > 90 || config.control[CONTROL_MAX_ELE_DEFLECTION] < 0) {
                snprintf(error, error_size, "Max elevator deflection must be between 0 and 90 degrees.");
                return false;
            }
            if (config.control[CONTROL_MAX_RUD_DEFLECTION] > 90 || config.control[CONTROL_MAX_RUD_DEFLECTION] < 0) {
                snprintf(error, error_size, "Max rudder deflection must be between 0 and 90 degrees.");
                return false;
            }
            break;
        case CTRLMODE_FLYINGWING_ATHR:
        case CTRLMODE_FLYINGWING:
            if (config.control[CONTROL_MAX_ELEVON_DEFLECTION] > 90 ||
                config.control[CONTROL_MAX_ELEVON_DEFLECTION] < 0) {
                snprintf(error, error_size, "Max elevon deflection must be between 0 and 90 degrees.");
                return false;
            }
            break;
    }
    // Wi-Fi ssid/password validation
    if (strlen(config.wifi.ssid) < WIFI_SSID_MIN_LEN || strlen(config.wifi.ssid) > WIFI_SSID_MAX_LEN) {
        snprintf(error, error_size, "Wi-Fi SSID must be between %d and %d characters.", WIFI_SSID_MIN_LEN,
                 WIFI_SSID_MAX_LEN);
        return false;
    }
    if (strlen(config.wifi.pass) > 0 &&
        (strlen(config.wifi.pass) < WIFI_PASS_MIN_LEN || strlen(config.wifi.pass) > WIFI_SSID_MAX_LEN)) {
        snprintf(error, error_size, "Wi-Fi password must be between %d and %d characters.", WIFI_PASS_MIN_LEN,
                 WIFI_PASS_MAX_LEN);
        return false;
    }
    return true;
}

ConfigSectionType config_get(const char *section, const char *key, void **value) {
    if (strcasecmp(section, CONFIG_GENERAL_STR) == 0) {
        f32 *v;
        get_from_general(key, &v);
        *value = v;
        return SECTION_TYPE_FLOAT;
    } else if (strcasecmp(section, CONFIG_CONTROL_STR) == 0) {
        f32 *v;
        get_from_control(key, &v);
        *value = v;
        return SECTION_TYPE_FLOAT;
    } else if (strcasecmp(section, CONFIG_PINS_STR) == 0) {
        f32 *v;
        get_from_pins(key, &v);
        *value = v;
        return SECTION_TYPE_FLOAT;
    } else if (strcasecmp(section, CONFIG_SENSORS_STR) == 0) {
        f32 *v;
        get_from_sensors(key, &v);
        *value = v;
        return SECTION_TYPE_FLOAT;
    } else if (strcasecmp(section, CONFIG_WIFI_STR) == 0) {
        char *v;
        get_from_wifi(key, &v);
        *value = v;
        return SECTION_TYPE_STRING;
    } else if (strcasecmp(section, CONFIG_SYSTEM_STR) == 0) {
        f32 *v;
        get_from_system(key, &v);
        *value = v;
        return SECTION_TYPE_FLOAT;
    } else {
        return SECTION_TYPE_NONE;
    }
}

ConfigSetResult config_set(const char *section, const char *key, const char *value) {
    if (strcasecmp(section, CONFIG_GENERAL_STR) == 0) {
        if (!set_to_general(key, (f32)atof(value))) {
            return CONFIG_SET_DOES_NOT_EXIST;
        }
    } else if (strcasecmp(section, CONFIG_CONTROL_STR) == 0) {
        if (!set_to_control(key, (f32)atof(value))) {
            return CONFIG_SET_DOES_NOT_EXIST;
        }
    } else if (strcasecmp(section, CONFIG_PINS_STR) == 0) {
        if (!set_to_pins(key, (f32)atof(value))) {
            return CONFIG_SET_DOES_NOT_EXIST;
        }
    } else if (strcasecmp(section, CONFIG_SENSORS_STR) == 0) {
        if (!set_to_sensors(key, (f32)atof(value))) {
            return CONFIG_SET_DOES_NOT_EXIST;
        }
    } else if (strcasecmp(section, CONFIG_WIFI_STR) == 0) {
        if (!set_to_wifi(key, value)) {
            return CONFIG_SET_DOES_NOT_EXIST;
        }
    } else if (strcasecmp(section, CONFIG_SYSTEM_STR) == 0) {
        if (!set_to_system(key, (f32)atof(value))) {
            return CONFIG_SET_DOES_NOT_EXIST;
        }
    } else {
        return CONFIG_SET_DOES_NOT_EXIST;
    }
    char error[128];
    bool valid = config_validate(error, sizeof(error));
    return valid ? CONFIG_SET_OK : CONFIG_SET_INVALID;
}

void config_backup() {
    backedUpConfig = config;
}

void config_restore() {
    config = backedUpConfig;
}

ConfigSectionType config_to_string(ConfigSection section, const char **str) {
    switch (section) {
        case CONFIG_GENERAL:
            *str = CONFIG_GENERAL_STR;
            return SECTION_TYPE_FLOAT;
        case CONFIG_CONTROL:
            *str = CONFIG_CONTROL_STR;
            return SECTION_TYPE_FLOAT;
        case CONFIG_PINS:
            *str = CONFIG_PINS_STR;
            return SECTION_TYPE_FLOAT;
        case CONFIG_SENSORS:
            *str = CONFIG_SENSORS_STR;
            return SECTION_TYPE_FLOAT;
        case CONFIG_WIFI:
            *str = CONFIG_WIFI_STR;
            return SECTION_TYPE_STRING;
        case CONFIG_SYSTEM:
            *str = CONFIG_SYSTEM_STR;
            return SECTION_TYPE_FLOAT;
    }
    *str = NULL;
    return SECTION_TYPE_NONE;
}
