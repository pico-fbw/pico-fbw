/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 *
 * I would much rather this file be named `config.c`, but the C compiler does not share this sentiment.
 * (I think there's a file called "sys/config.h" in GCC so it gets mad? Annoying.)
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "platform/flash.h"
#include "platform/int.h"

#include "io/aahrs.h"
#include "io/gps.h"
#include "io/receiver.h"

#include "sys/print.h"
#include "sys/runtime.h"
#include "sys/version.h"

#include "configuration.h"

// clang-format off
Config config = {
    // FIXME: index 7 (false) of general is the wifi config flag, replace this with an enum when the wifi HAL is done and has one
    .general = {
        CTRLMODE_2AXIS_ATHR, SWITCH_TYPE_3_POS, 20, 50, 50, true, false, false, CONFIG_END_MAGIC,
    },
    .control = {
        0.0025f, 1.5f, 5, // Control handling preferences
        10, 30, 0.015f, // Autothrottle configuration
        180, 0, // Drop bay detent settings
        33, 67, -15, 30, // Control limits
        25, 15, 20, // Physical control surface limits
        20, 0.5f, 1, 1, // Flying wing configuration
        CONFIG_END_MAGIC,
    },
    .pins = {
        PIN_INPUT_AIL, PIN_SERVO_AIL, PIN_INPUT_ELE, PIN_SERVO_ELE, PIN_INPUT_RUD,
        PIN_SERVO_RUD, PIN_INPUT_THR, PIN_ESC_THR, PIN_INPUT_SWITCH, PIN_SERVO_BAY, // Control IO pins
        PIN_AAHRS_SDA, PIN_AAHRS_SCL, PIN_GPS_TX, PIN_GPS_RX, // Sensor communications pins
        false, false, false, // Servo reverse flags
        CONFIG_END_MAGIC,
    },
    .sensors = {
        IMU_MODEL_BNO055, BARO_MODEL_NONE, GPS_COMMAND_TYPE_PMTK, 9600,
        CONFIG_END_MAGIC,
    },
    .system = {
        false,
        true, false, false, false, false, false, // Default print settings, also found in PrintDefs below
        2000,
        CONFIG_END_MAGIC,
    },
    .ssid = "pico-fbw",
    .pass = "picodashfbw"
};

Calibration calibration = {
    // Only throttle detents have default values
    .esc = {
        false, 10, 75, 90
    }
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
        if (lfs_file_open(&lfs, &f, file, LFS_O_RDWR | LFS_O_CREAT) != LFS_ERR_OK)
            return false;
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
    if (lfs_file_open(&lfs, &f, file, LFS_O_WRONLY | LFS_O_TRUNC) != LFS_ERR_OK)
        return false;
    if (lfs_file_write(&lfs, &f, strct, size) != (lfs_ssize_t)size) {
        lfs_file_close(&lfs, &f);
        return false;
    }
    return lfs_file_close(&lfs, &f) == LFS_ERR_OK;
}

void config_load() {
    load_file_to_struct("config", &config, sizeof(config));
    load_file_to_struct("calibration", &calibration, sizeof(calibration));
    // Load print settings and set debug flag
    shouldPrint.fbw = config.system[SYSTEM_PRINT_FBW];
    shouldPrint.aahrs = config.system[SYSTEM_PRINT_AAHRS];
    shouldPrint.gps = config.system[SYSTEM_PRINT_GPS];
    shouldPrint.modes = config.system[SYSTEM_PRINT_MODES];
    shouldPrint.network = config.system[SYSTEM_PRINT_NETWORK];
    config.system[SYSTEM_DEBUG] = DEBUG_BUILD;
}

void config_save() {
    save_struct_to_file("config", &config, sizeof(config));
    save_struct_to_file("calibration", &calibration, sizeof(calibration));
}

void config_reset() {
    lfs_remove(&lfs, "config");
    lfs_remove(&lfs, "calibration");
}

// I'm so sorry (this is a C moment)

static void get_from_general(const char *key, float **value) {
    if (strcasecmp(key, "controlMode") == 0) {
        *value = &config.general[GENERAL_CONTROL_MODE];
    } else if (strcasecmp(key, "switchType") == 0) {
        *value = &config.general[GENERAL_SWITCH_TYPE];
    } else if (strcasecmp(key, "maxCalibrationOffset") == 0) {
        *value = &config.general[GENERAL_MAX_CALIBRATION_OFFSET];
    } else if (strcasecmp(key, "servoHz") == 0) {
        *value = &config.general[GENERAL_SERVO_HZ];
    } else if (strcasecmp(key, "escHz") == 0) {
        *value = &config.general[GENERAL_ESC_HZ];
    } else if (strcasecmp(key, "apiEnabled") == 0) {
        *value = &config.general[GENERAL_API_ENABLED];
    } else if (strcasecmp(key, "wifiEnabled") == 0) {
        *value = &config.general[GENERAL_WIFI_ENABLED];
    } else if (strcasecmp(key, "skipCalibration") == 0) {
        *value = &config.general[GENERAL_SKIP_CALIBRATION];
    } else {
        *value = NULL;
    }
}

static bool set_to_general(const char *key, float value) {
    if (strcasecmp(key, "controlMode") == 0) {
        config.general[GENERAL_CONTROL_MODE] = value;
    } else if (strcasecmp(key, "switchType") == 0) {
        config.general[GENERAL_SWITCH_TYPE] = value;
    } else if (strcasecmp(key, "maxCalibrationOffset") == 0) {
        config.general[GENERAL_MAX_CALIBRATION_OFFSET] = value;
    } else if (strcasecmp(key, "servoHz") == 0) {
        config.general[GENERAL_SERVO_HZ] = value;
    } else if (strcasecmp(key, "escHz") == 0) {
        config.general[GENERAL_ESC_HZ] = value;
    } else if (strcasecmp(key, "apiEnabled") == 0) {
        config.general[GENERAL_API_ENABLED] = value;
    } else if (strcasecmp(key, "wifiEnabled") == 0) {
        config.general[GENERAL_WIFI_ENABLED] = value;
    } else if (strcasecmp(key, "skipCalibration") == 0) {
        config.general[GENERAL_SKIP_CALIBRATION] = value;
    } else
        return false;
    return true;
}

static void get_from_control(const char *key, float **value) {
    if (strcasecmp(key, "controlSensitivity") == 0) {
        *value = &config.control[CONTROL_SENSITIVITY];
    } else if (strcasecmp(key, "rudderSensitivity") == 0) {
        *value = &config.control[CONTROL_RUDDER_SENSITIVITY];
    } else if (strcasecmp(key, "controlDeadband") == 0) {
        *value = &config.control[CONTROL_DEADBAND];
    } else if (strcasecmp(key, "throttleMaxTime") == 0) {
        *value = &config.control[CONTROL_THROTTLE_MAX_TIME];
    } else if (strcasecmp(key, "throttleCooldownTime") == 0) {
        *value = &config.control[CONTROL_THROTTLE_COOLDOWN_TIME];
    } else if (strcasecmp(key, "throttleSensitivity") == 0) {
        *value = &config.control[CONTROL_THROTTLE_SENSITIVITY];
    } else if (strcasecmp(key, "dropDetentClosed") == 0) {
        *value = &config.control[CONTROL_DROP_DETENT_CLOSED];
    } else if (strcasecmp(key, "dropDetentOpen") == 0) {
        *value = &config.control[CONTROL_DROP_DETENT_OPEN];
    } else if (strcasecmp(key, "rollLimit") == 0) {
        *value = &config.control[CONTROL_ROLL_LIMIT];
    } else if (strcasecmp(key, "rollLimitHold") == 0) {
        *value = &config.control[CONTROL_ROLL_LIMIT_HOLD];
    } else if (strcasecmp(key, "pitchLowerLimit") == 0) {
        *value = &config.control[CONTROL_PITCH_LOWER_LIMIT];
    } else if (strcasecmp(key, "pitchUpperLimit") == 0) {
        *value = &config.control[CONTROL_PITCH_UPPER_LIMIT];
    } else if (strcasecmp(key, "maxAilDeflection") == 0) {
        *value = &config.control[CONTROL_MAX_AIL_DEFLECTION];
    } else if (strcasecmp(key, "maxElevDeflection") == 0) {
        *value = &config.control[CONTROL_MAX_ELEV_DEFLECTION];
    } else if (strcasecmp(key, "maxRudDeflection") == 0) {
        *value = &config.control[CONTROL_MAX_RUD_DEFLECTION];
    } else if (strcasecmp(key, "maxElevonDeflection") == 0) {
        *value = &config.control[CONTROL_MAX_ELEVON_DEFLECTION];
    } else if (strcasecmp(key, "elevonMixingGain") == 0) {
        *value = &config.control[CONTROL_ELEVON_MIXING_GAIN];
    } else if (strcasecmp(key, "ailMixingBias") == 0) {
        *value = &config.control[CONTROL_AIL_MIXING_BIAS];
    } else if (strcasecmp(key, "elevMixingBias") == 0) {
        *value = &config.control[CONTROL_ELEV_MIXING_BIAS];
    } else {
        *value = NULL;
    }
}

static bool set_to_control(const char *key, float value) {
    if (strcasecmp(key, "controlSensitivity") == 0) {
        config.control[CONTROL_SENSITIVITY] = value;
    } else if (strcasecmp(key, "rudderSensitivity") == 0) {
        config.control[CONTROL_RUDDER_SENSITIVITY] = value;
    } else if (strcasecmp(key, "controlDeadband") == 0) {
        config.control[CONTROL_DEADBAND] = value;
    } else if (strcasecmp(key, "throttleMaxTime") == 0) {
        config.control[CONTROL_THROTTLE_MAX_TIME] = value;
    } else if (strcasecmp(key, "throttleCooldownTime") == 0) {
        config.control[CONTROL_THROTTLE_COOLDOWN_TIME] = value;
    } else if (strcasecmp(key, "throttleSensitivity") == 0) {
        config.control[CONTROL_THROTTLE_SENSITIVITY] = value;
    } else if (strcasecmp(key, "dropDetentClosed") == 0) {
        config.control[CONTROL_DROP_DETENT_CLOSED] = value;
    } else if (strcasecmp(key, "dropDetentOpen") == 0) {
        config.control[CONTROL_DROP_DETENT_OPEN] = value;
    } else if (strcasecmp(key, "rollLimit") == 0) {
        config.control[CONTROL_ROLL_LIMIT] = value;
    } else if (strcasecmp(key, "rollLimitHold") == 0) {
        config.control[CONTROL_ROLL_LIMIT_HOLD] = value;
    } else if (strcasecmp(key, "pitchLowerLimit") == 0) {
        config.control[CONTROL_PITCH_LOWER_LIMIT] = value;
    } else if (strcasecmp(key, "pitchUpperLimit") == 0) {
        config.control[CONTROL_PITCH_UPPER_LIMIT] = value;
    } else if (strcasecmp(key, "maxAilDeflection") == 0) {
        config.control[CONTROL_MAX_AIL_DEFLECTION] = value;
    } else if (strcasecmp(key, "maxElevDeflection") == 0) {
        config.control[CONTROL_MAX_ELEV_DEFLECTION] = value;
    } else if (strcasecmp(key, "maxRudDeflection") == 0) {
        config.control[CONTROL_MAX_RUD_DEFLECTION] = value;
    } else if (strcasecmp(key, "maxElevonDeflection") == 0) {
        config.control[CONTROL_MAX_ELEVON_DEFLECTION] = value;
    } else if (strcasecmp(key, "elevonMixingGain") == 0) {
        config.control[CONTROL_ELEVON_MIXING_GAIN] = value;
    } else if (strcasecmp(key, "ailMixingBias") == 0) {
        config.control[CONTROL_AIL_MIXING_BIAS] = value;
    } else if (strcasecmp(key, "elevMixingBias") == 0) {
        config.control[CONTROL_ELEV_MIXING_BIAS] = value;
    } else
        return false;
    return true;
}

static void get_from_pins(const char *key, float **value) {
    if (strcasecmp(key, "inputAil") == 0) {
        *value = &config.pins[PINS_INPUT_AIL];
    } else if (strcasecmp(key, "servoAil") == 0) {
        *value = &config.pins[PINS_SERVO_AIL];
    } else if (strcasecmp(key, "inputElev") == 0) {
        *value = &config.pins[PINS_INPUT_ELE];
    } else if (strcasecmp(key, "servoElev") == 0) {
        *value = &config.pins[PINS_SERVO_ELE];
    } else if (strcasecmp(key, "inputRud") == 0) {
        *value = &config.pins[PINS_INPUT_RUD];
    } else if (strcasecmp(key, "servoRud") == 0) {
        *value = &config.pins[PINS_SERVO_RUD];
    } else if (strcasecmp(key, "inputThrottle") == 0) {
        *value = &config.pins[PINS_INPUT_THROTTLE];
    } else if (strcasecmp(key, "escThrottle") == 0) {
        *value = &config.pins[PINS_ESC_THROTTLE];
    } else if (strcasecmp(key, "inputSwitch") == 0) {
        *value = &config.pins[PINS_INPUT_SWITCH];
    } else if (strcasecmp(key, "servoBay") == 0) {
        *value = &config.pins[PINS_SERVO_BAY];
    } else if (strcasecmp(key, "aahrsSda") == 0) {
        *value = &config.pins[PINS_AAHRS_SDA];
    } else if (strcasecmp(key, "aahrsScl") == 0) {
        *value = &config.pins[PINS_AAHRS_SCL];
    } else if (strcasecmp(key, "gpsTx") == 0) {
        *value = &config.pins[PINS_GPS_TX];
    } else if (strcasecmp(key, "gpsRx") == 0) {
        *value = &config.pins[PINS_GPS_RX];
    } else if (strcasecmp(key, "reverseRoll") == 0) {
        *value = &config.pins[PINS_REVERSE_ROLL];
    } else if (strcasecmp(key, "reversePitch") == 0) {
        *value = &config.pins[PINS_REVERSE_PITCH];
    } else if (strcasecmp(key, "reverseYaw") == 0) {
        *value = &config.pins[PINS_REVERSE_YAW];
    } else {
        *value = NULL;
    }
}

static bool set_to_pins(const char *key, float value) {
    if (strcasecmp(key, "inputAil") == 0) {
        config.pins[PINS_INPUT_AIL] = value;
    } else if (strcasecmp(key, "servoAil") == 0) {
        config.pins[PINS_SERVO_AIL] = value;
    } else if (strcasecmp(key, "inputElev") == 0) {
        config.pins[PINS_INPUT_ELE] = value;
    } else if (strcasecmp(key, "servoElev") == 0) {
        config.pins[PINS_SERVO_ELE] = value;
    } else if (strcasecmp(key, "inputRud") == 0) {
        config.pins[PINS_INPUT_RUD] = value;
    } else if (strcasecmp(key, "servoRud") == 0) {
        config.pins[PINS_SERVO_RUD] = value;
    } else if (strcasecmp(key, "inputThrottle") == 0) {
        config.pins[PINS_INPUT_THROTTLE] = value;
    } else if (strcasecmp(key, "escThrottle") == 0) {
        config.pins[PINS_ESC_THROTTLE] = value;
    } else if (strcasecmp(key, "inputSwitch") == 0) {
        config.pins[PINS_INPUT_SWITCH] = value;
    } else if (strcasecmp(key, "servoBay") == 0) {
        config.pins[PINS_SERVO_BAY] = value;
    } else if (strcasecmp(key, "aahrsSda") == 0) {
        config.pins[PINS_AAHRS_SDA] = value;
    } else if (strcasecmp(key, "aahrsScl") == 0) {
        config.pins[PINS_AAHRS_SCL] = value;
    } else if (strcasecmp(key, "gpsTx") == 0) {
        config.pins[PINS_GPS_TX] = value;
    } else if (strcasecmp(key, "gpsRx") == 0) {
        config.pins[PINS_GPS_RX] = value;
    } else if (strcasecmp(key, "reverseRoll") == 0) {
        config.pins[PINS_REVERSE_ROLL] = value;
    } else if (strcasecmp(key, "reversePitch") == 0) {
        config.pins[PINS_REVERSE_PITCH] = value;
    } else if (strcasecmp(key, "reverseYaw") == 0) {
        config.pins[PINS_REVERSE_YAW] = value;
    } else
        return false;
    return true;
}

static void get_from_sensors(const char *key, float **value) {
    if (strcasecmp(key, "imuModel") == 0) {
        *value = &config.sensors[SENSORS_IMU_MODEL];
    } else if (strcasecmp(key, "baroModel") == 0) {
        *value = &config.sensors[SENSORS_BARO_MODEL];
    } else if (strcasecmp(key, "gpsCommandType") == 0) {
        *value = &config.sensors[SENSORS_GPS_COMMAND_TYPE];
    } else if (strcasecmp(key, "gpsBaudrate") == 0) {
        *value = &config.sensors[SENSORS_GPS_BAUDRATE];
    } else {
        *value = NULL;
    }
}

static bool set_to_sensors(const char *key, float value) {
    if (strcasecmp(key, "imuModel") == 0) {
        config.sensors[SENSORS_IMU_MODEL] = value;
    } else if (strcasecmp(key, "baroModel") == 0) {
        config.sensors[SENSORS_BARO_MODEL] = value;
    } else if (strcasecmp(key, "gpsCommandType") == 0) {
        config.sensors[SENSORS_GPS_COMMAND_TYPE] = value;
    } else if (strcasecmp(key, "gpsBaudrate") == 0) {
        config.sensors[SENSORS_GPS_BAUDRATE] = value;
    } else
        return false;
    return true;
}

static void get_from_system(const char *key, float **value) {
    if (strcasecmp(key, "debug") == 0) {
        *value = &config.system[SYSTEM_DEBUG];
    } else if (strcasecmp(key, "printFBW") == 0) {
        *value = &config.system[SYSTEM_PRINT_FBW];
    } else if (strcasecmp(key, "printAAHRS") == 0) {
        *value = &config.system[SYSTEM_PRINT_AAHRS];
    } else if (strcasecmp(key, "printGPS") == 0) {
        *value = &config.system[SYSTEM_PRINT_GPS];
    } else if (strcasecmp(key, "printModes") == 0) {
        *value = &config.system[SYSTEM_PRINT_MODES];
    } else if (strcasecmp(key, "printNetwork") == 0) {
        *value = &config.system[SYSTEM_PRINT_NETWORK];
    } else {
        *value = NULL;
    }
}

static bool set_to_system(const char *key, float value) {
    if (strcasecmp(key, "printFBW") == 0) {
        config.system[SYSTEM_PRINT_FBW] = value;
    } else if (strcasecmp(key, "printAAHRS") == 0) {
        config.system[SYSTEM_PRINT_AAHRS] = value;
    } else if (strcasecmp(key, "printGPS") == 0) {
        config.system[SYSTEM_PRINT_GPS] = value;
    } else if (strcasecmp(key, "printModes") == 0) {
        config.system[SYSTEM_PRINT_MODES] = value;
    } else if (strcasecmp(key, "printNetwork") == 0) {
        config.system[SYSTEM_PRINT_NETWORK] = value;
    } else
        return false;
    return true;
}

static void get_from_wifi(const char *key, char **value) {
    if (strcasecmp(key, "ssid") == 0) {
        *value = config.ssid;
    } else if (strcasecmp(key, "pass") == 0) {
        *value = config.pass;
    } else {
        *value = NULL;
    }
}

static bool set_to_wifi(const char *key, const char *value) {
    if (strcasecmp(key, "ssid") == 0) {
        strcpy(config.ssid, value);
    } else if (strcasecmp(key, "pass") == 0) {
        strcpy(config.pass, value);
    } else
        return false;
    return true;
}

bool config_validate() {
    // Unique pin validation
    i32 lastPin = -1;
    switch ((ControlMode)config.general[GENERAL_CONTROL_MODE]) {
        case CTRLMODE_3AXIS_ATHR:
            for (u32 i = S_PIN_MIN; i <= S_PIN_MAX; i++) {
                if ((i32)config.pins[i] == lastPin)
                    goto invalid;
            }
            break;
        case CTRLMODE_3AXIS:
            for (u32 i = S_PIN_MIN; i <= S_PIN_MAX; i++) {
                // Skip pins that aren't utilized in this mode
                if (i == PINS_INPUT_THROTTLE || i == PINS_ESC_THROTTLE)
                    break;
                if ((i32)config.pins[i] == lastPin)
                    goto invalid;
            }
            break;
        case CTRLMODE_2AXIS_ATHR:
        case CTRLMODE_FLYINGWING_ATHR:
            for (u32 i = S_PIN_MIN; i <= S_PIN_MAX; i++) {
                if (i == PINS_INPUT_RUD)
                    break;
                if ((i32)config.pins[i] == lastPin)
                    goto invalid;
            }
            break;
        case CTRLMODE_2AXIS:
        case CTRLMODE_FLYINGWING:
            for (u32 i = S_PIN_MIN; i <= S_PIN_MAX; i++) {
                if (i == PINS_INPUT_RUD || i == PINS_INPUT_THROTTLE || i == PINS_ESC_THROTTLE)
                    break;
                if ((i32)config.pins[i] == lastPin)
                    goto invalid;
            }
            break;
        invalid:
            print("ERROR: A pin may only be used once.");
            return false;
    }
    // Sensor pin validation
    // AAHRS_SDA can be on pins 0, 4, 8, 12, 16, 20, 28
    if ((u32)config.pins[PINS_AAHRS_SDA] != 0 && (u32)config.pins[PINS_AAHRS_SDA] != 4 &&
        (u32)config.pins[PINS_AAHRS_SDA] != 8 && (u32)config.pins[PINS_AAHRS_SDA] != 12 &&
        (u32)config.pins[PINS_AAHRS_SDA] != 16 && (u32)config.pins[PINS_AAHRS_SDA] != 20 &&
        (u32)config.pins[PINS_AAHRS_SDA] != 28) {
        print("ERROR: IMU_SDA must be on the I2C0_SDA interface.");
        return false;
    }
    // AAHRS_SCL can be on pins 1, 5, 9, 13, 17, 21
    if ((u32)config.pins[PINS_AAHRS_SCL] != 1 && (u32)config.pins[PINS_AAHRS_SCL] != 5 &&
        (u32)config.pins[PINS_AAHRS_SCL] != 9 && (u32)config.pins[PINS_AAHRS_SCL] != 13 &&
        (u32)config.pins[PINS_AAHRS_SCL] != 17 && (u32)config.pins[PINS_AAHRS_SCL] != 21) {
        print("ERROR: IMU_SCL must be on the I2C0_SCL interface.");
        return false;
    }
    // GPS_RX can be on pins 4, 8, 20
    if ((u32)config.pins[PINS_GPS_RX] != 4 && (u32)config.pins[PINS_GPS_RX] != 8 && (u32)config.pins[PINS_GPS_RX] != 20) {
        print("ERROR: GPS_RX must be on the UART1_RX interface.");
        return false;
    }
    // GPS_TX can be on pins 5, 9, 21
    if ((u32)config.pins[PINS_GPS_TX] != 5 && (u32)config.pins[PINS_GPS_TX] != 9 && (u32)config.pins[PINS_GPS_TX] != 21) {
        print("ERROR: GPS_TX must be on the UART1_TX interface.");
        return false;
    }
    // Limits validation
    if (config.control[CONTROL_ROLL_LIMIT] > 72 || config.control[CONTROL_ROLL_LIMIT] < 0) {
        print("ERROR: Roll limit must be between 0 and 72 degrees.");
        return false;
    }
    if (config.control[CONTROL_ROLL_LIMIT_HOLD] > 72 || config.control[CONTROL_ROLL_LIMIT_HOLD] < 0) {
        print("ERROR: Roll limit hold must be between 0 and 72 degrees.");
        return false;
    }
    if (config.control[CONTROL_PITCH_UPPER_LIMIT] > 35 || config.control[CONTROL_PITCH_UPPER_LIMIT] < 0) {
        print("ERROR: Upper pitch limit must be between 0 and 35 degrees.");
        return false;
    }
    if (config.control[CONTROL_PITCH_LOWER_LIMIT] < -20 || config.control[CONTROL_PITCH_LOWER_LIMIT] > 0) {
        print("ERROR: Lower pitch limit must be between -20 and 0 degrees.");
        return false;
    }
    // Sensor model validation
    if (config.sensors[SENSORS_IMU_MODEL] < IMU_MODEL_MIN || config.sensors[SENSORS_IMU_MODEL] >= IMU_MODEL_MAX) {
        print("ERROR: IMU model must be between %d and %d.", IMU_MODEL_MIN, IMU_MODEL_MAX - 1);
        return false;
    }
    if (config.sensors[SENSORS_BARO_MODEL] < BARO_MODEL_MIN || config.sensors[SENSORS_BARO_MODEL] >= BARO_MODEL_MAX) {
        print("ERROR: Barometer model must be between %d and %d.", BARO_MODEL_MIN, BARO_MODEL_MAX - 1);
        return false;
    }
    if (config.sensors[SENSORS_GPS_COMMAND_TYPE] < GPS_COMMAND_TYPE_MIN ||
        config.sensors[SENSORS_GPS_COMMAND_TYPE] >= GPS_COMMAND_TYPE_MAX) {
        print("ERROR: GPS command type must be between %d and %d.", GPS_COMMAND_TYPE_MIN, GPS_COMMAND_TYPE_MAX - 1);
        return false;
    }
    // Throttle  configuration validation
    if (config.control[CONTROL_THROTTLE_SENSITIVITY] < 0.0f || config.control[CONTROL_THROTTLE_SENSITIVITY] > 1.0f) {
        print("ERROR: Throttle sensitivity must be between 0.0 and 1.0.");
        return false;
    }
    // Drop (servo position) validation
    if (config.control[CONTROL_DROP_DETENT_CLOSED] < 0 || config.control[CONTROL_DROP_DETENT_OPEN] > 180) {
        print("ERROR: Drop detent (closed) must be between 0 and 180 degrees.");
        return false;
    }
    if (config.control[CONTROL_DROP_DETENT_OPEN] < 0 || config.control[CONTROL_DROP_DETENT_OPEN] > 180) {
        print("ERROR: Drop detent (open) must be between 0 and 180 degrees.");
        return false;
    }
    // Control limit validation
    switch ((ControlMode)config.general[GENERAL_CONTROL_MODE]) {
        case CTRLMODE_3AXIS_ATHR:
        case CTRLMODE_3AXIS:
        case CTRLMODE_2AXIS_ATHR:
        case CTRLMODE_2AXIS:
            if (config.control[CONTROL_MAX_AIL_DEFLECTION] > 90 || config.control[CONTROL_MAX_AIL_DEFLECTION] < 0) {
                print("ERROR: Max aileron deflection must be between 0 and 90 degrees.");
                return false;
            }
            if (config.control[CONTROL_MAX_ELEV_DEFLECTION] > 90 || config.control[CONTROL_MAX_ELEV_DEFLECTION] < 0) {
                print("ERROR: Max elevator deflection must be between 0 and 90 degrees.");
                return false;
            }
            if (config.control[CONTROL_MAX_RUD_DEFLECTION] > 90 || config.control[CONTROL_MAX_RUD_DEFLECTION] < 0) {
                print("ERROR: Max rudder deflection must be between 0 and 90 degrees.");
                return false;
            }
            break;
        case CTRLMODE_FLYINGWING_ATHR:
        case CTRLMODE_FLYINGWING:
            if (config.control[CONTROL_MAX_ELEVON_DEFLECTION] > 90 || config.control[CONTROL_MAX_ELEVON_DEFLECTION] < 0) {
                print("ERROR: Max elevon deflection must be between 0 and 90 degrees.");
                return false;
            }
            break;
    }
    return true;
}

ConfigSectionType config_get(const char *section, const char *key, void **value) {
    if (strcasecmp(section, CONFIG_GENERAL_STR) == 0) {
        float *v;
        get_from_general(key, &v);
        *value = v;
        return SECTION_TYPE_FLOAT;
    } else if (strcasecmp(section, CONFIG_CONTROL_STR) == 0) {
        float *v;
        get_from_control(key, &v);
        *value = v;
        return SECTION_TYPE_FLOAT;
    } else if (strcasecmp(section, CONFIG_PINS_STR) == 0) {
        float *v;
        get_from_pins(key, &v);
        *value = v;
        return SECTION_TYPE_FLOAT;
    } else if (strcasecmp(section, CONFIG_SENSORS_STR) == 0) {
        float *v;
        get_from_sensors(key, &v);
        *value = v;
        return SECTION_TYPE_FLOAT;
    } else if (strcasecmp(section, CONFIG_WIFI_STR) == 0) {
        char *v;
        get_from_wifi(key, &v);
        *value = v;
        return SECTION_TYPE_STRING;
    } else if (strcasecmp(section, CONFIG_SYSTEM_STR) == 0) {
        float *v;
        get_from_system(key, &v);
        *value = v;
        return SECTION_TYPE_FLOAT;
    } else {
        return SECTION_TYPE_NONE;
    }
}

bool config_set(const char *section, const char *key, const char *value) {
    if (strcasecmp(section, CONFIG_GENERAL_STR) == 0) {
        if (!set_to_general(key, atoff(value)))
            return false;
    } else if (strcasecmp(section, CONFIG_CONTROL_STR) == 0) {
        if (!set_to_control(key, atoff(value)))
            return false;
    } else if (strcasecmp(section, CONFIG_PINS_STR) == 0) {
        if (!set_to_pins(key, atoff(value)))
            return false;
    } else if (strcasecmp(section, CONFIG_SENSORS_STR) == 0) {
        if (!set_to_sensors(key, atoff(value)))
            return false;
    } else if (strcasecmp(section, CONFIG_WIFI_STR) == 0) {
        if (!set_to_wifi(key, value))
            return false;
    } else if (strcasecmp(section, CONFIG_SYSTEM_STR) == 0) {
        if (!set_to_system(key, atoff(value)))
            return false;
    } else
        return false;
    return config_validate();
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
