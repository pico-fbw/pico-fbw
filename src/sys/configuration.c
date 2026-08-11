/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform/defs.h"
#include "platform/flash.h"
#include "platform/helpers.h"
#include "platform/wifi.h"

#include "ctrl/switch.h"
#include "io/gps.h"
#include "io/receiver.h"
#include "lib/drivers/drivers.h"
#include "lib/parson.h"
#include "sys/print.h"
#include "sys/runtime.h"
#include "sys/version.h"

#include "configuration.h"

#define FILE_CONFIG "config.json"
#define FILE_CALIBRATION "calibration.json"

static Config backedUpConfig;

// -- Default configuration values --

// clang-format off

Config config = {
    .general = {
        .controlMode = CTRLMODE_2AXIS_ATHR,
        .switchType = SWITCH_TYPE_3_POS,
        .maxCalibrationOffset = 20,
        .servoHz = 50,
        .escHz = 50,
        .apiEnabled = true,
// Wi-Fi should be enabled by default, but only if the platform supports it
#if PLATFORM_SUPPORTS_WIFI
        .wifiEnabled = WIFI_ENABLED_PASS,
#else
        .wifiEnabled = WIFI_DISABLED,
#endif
        .launchAssistEnabled = false,
        .autoTuneEnabled = true,
// Host platforms don't have the necessary hardware to pass calibration, so skip it by default
#if FBW_PLATFORM_HOST
        .skipCalibration = true,
#else
        .skipCalibration = false,
#endif
    },
    .control = {
        // Control handling preferences
        .maxRollRate = 50,
        .maxPitchRate = 20,
        .expo = 0.4f,
        .rudderSensitivity = 0.3f,
        .controlDeadband = 2.f,
        // Autothrottle configuration
        .throttleMaxTime = 10,
        .throttleCooldownTime = 30,
        .throttleSensitivity = 0.3f,
        // Drop bay detent settings
        .dropDetentClosed = 180,
        .dropDetentOpen = 0,
        // Control limits
        .rollLimit = 33,
        .rollLimitHold = 67,
        .pitchLowerLimit = -15,
        .pitchUpperLimit = 30,
        // Physical control surface limits
        .maxAilDeflection = 60,
        .maxEleDeflection = 60,
        .maxRudDeflection = 40,
        // Flying wing configuration
        .maxElevonDeflection = 20,
        .elevonMixingGain = 0.5f,
        .ailMixingBias = 1,
        .elevMixingBias = 1,
    },
    .pins = {
        // Control IO pins
        .inputAil = DEFAULT_PIN_INPUT_AIL,
        .servoAil = DEFAULT_PIN_SERVO_AIL,
        .inputEle = DEFAULT_PIN_INPUT_ELE,
        .servoEle = DEFAULT_PIN_SERVO_ELE,
        .inputRud = DEFAULT_PIN_INPUT_RUD,
        .servoRud = DEFAULT_PIN_SERVO_RUD,
        .inputThrottle = DEFAULT_PIN_INPUT_THR,
        .escThrottle = DEFAULT_PIN_ESC_THR,
        .inputSwitch = DEFAULT_PIN_INPUT_SWITCH,
        .servoBay = DEFAULT_PIN_SERVO_BAY,
        // Sensor communication pins
        .i2cSda = DEFAULT_PIN_I2C_SDA,
        .i2cScl = DEFAULT_PIN_I2C_SCL,
        .spiClk = DEFAULT_PIN_SPI_CLK,
        .spiMosi = DEFAULT_PIN_SPI_MOSI,
        .spiMiso = DEFAULT_PIN_SPI_MISO,
        .spiCs0 = DEFAULT_PIN_SPI_CS,
        .spiCs1 = -1,
        .spiCs2 = -1,
        .gpsTx = DEFAULT_PIN_GPS_TX,
        .gpsRx = DEFAULT_PIN_GPS_RX,
        // Servo reverse flags
        .reverseRoll = false,
        .reversePitch = false,
        .reverseYaw = false,
    },
    .sensors = {
        // I2C/SPI configuration
        .busType = BUS_I2C,
        .i2cBusFreq = 400,
        .spiBusFreq = 1,
        // GPS configuration
        .gpsCommandType = GPS_COMMAND_TYPE_PMTK,
        .gpsBaudrate = 9600,
    },
    .system = {
        .ssid = "pico-fbw",
        .pass = "picodashfbw",
        // Default print settings, also found in PrintDefs below
        .print = true,
        .printIMU = false,
        .printAircraft = false,
        .printGPS = false,
        .printNetwork = false,
    },
    .webui = {
        .defaultSpeed = "25",
        .dropSecs = "10",
        // Internal settings, cannot be changed by user
        .pilotName = "",
        .defaultMap = "0",
        .lastMapPosition = "",
        .lastMapZoom = "",
        .setupComplete = "false",
    },
    .version = CONFIG_VERSION,
};

Calibration calibration = {
    .pwm = {
        .calibrated = false,
        .mode = CTRLMODE_2AXIS_ATHR,
        // Default PWM offsets
        .offsetAil = 0,
        .offsetEle = 0,
        .offsetRud = 0,
        .offsetSw = 0,
        .offsetThr = 0,
    },
    .esc = {
        .calibrated = false,
        // Default throttle detents
        .detentIdle = 10,
        .detentMct = 75,
        .detentMax = 90,
    },
    .imu = {
        .calibrated = false,
        // Default gyro bias
        .gyroBiasX = 0,
        .gyroBiasY = 0,
        .gyroBiasZ = 0,
        // Default accel offset
        .accelOffsetX = 0,
        .accelOffsetY = 0,
        .accelOffsetZ = 0,
        // Default axis map (identity)
        .axisMapRoll = 0,
        .axisMapPitch = 1,
        .axisMapYaw = 2,
        // Default axis signs
        .axisSignRoll = 1,
        .axisSignPitch = 1,
        .axisSignYaw = 1,
    },
    .pid = {
        .tuned = false,
        // Default roll PID parameters
        .rollKp = 1.f,
        .rollKi = 0.2f,
        .rollKd = 1.f,
        .rollDb = 0.75f,
        // Default pitch PID parameters
        .pitchKp = 1.f,
        .pitchKi = 0.2f,
        .pitchKd = 1.f,
        .pitchDb = 0.75f,
        // Default yaw PID parameters
        .yawKp = 0.15f,
        .yawKi = 0.05f,
        .yawKd = 0.f,
        .yawDb = 1.5f,
        // Default autothrottle PID parameters
        .throttleKp = 6.f,
        .throttleKi = 0.3f,
        .throttleKd = 0.f,
        // Default PID tau
        .tau = 1.f,
    },
    .version = CALIBRATION_VERSION,
};

// clang-format on

PrintDefs shouldPrint = {
    // Default print settings
    true, false, false, false, false,
};

// -- `config` and `calibration` entries for use in lookup functions --

static ConfigEntry configGeneral[] = {
    {"controlMode", SECTION_TYPE_NUMBER, &config.general.controlMode},
    {"switchType", SECTION_TYPE_NUMBER, &config.general.switchType},
    {"maxCalibrationOffset", SECTION_TYPE_NUMBER, &config.general.maxCalibrationOffset},
    {"servoHz", SECTION_TYPE_NUMBER, &config.general.servoHz},
    {"escHz", SECTION_TYPE_NUMBER, &config.general.escHz},
    {"apiEnabled", SECTION_TYPE_NUMBER, &config.general.apiEnabled},
    {"wifiEnabled", SECTION_TYPE_NUMBER, &config.general.wifiEnabled},
    {"launchAssistEnabled", SECTION_TYPE_NUMBER, &config.general.launchAssistEnabled},
    {"autoTuneEnabled", SECTION_TYPE_NUMBER, &config.general.autoTuneEnabled},
    {"skipCalibration", SECTION_TYPE_NUMBER, &config.general.skipCalibration},
};
static ConfigEntry configControl[] = {
    {"maxRollRate", SECTION_TYPE_NUMBER, &config.control.maxRollRate},
    {"maxPitchRate", SECTION_TYPE_NUMBER, &config.control.maxPitchRate},
    {"expo", SECTION_TYPE_NUMBER, &config.control.expo},
    {"rudderSensitivity", SECTION_TYPE_NUMBER, &config.control.rudderSensitivity},
    {"controlDeadband", SECTION_TYPE_NUMBER, &config.control.controlDeadband},
    {"throttleMaxTime", SECTION_TYPE_NUMBER, &config.control.throttleMaxTime},
    {"throttleCooldownTime", SECTION_TYPE_NUMBER, &config.control.throttleCooldownTime},
    {"throttleSensitivity", SECTION_TYPE_NUMBER, &config.control.throttleSensitivity},
    {"dropDetentClosed", SECTION_TYPE_NUMBER, &config.control.dropDetentClosed},
    {"dropDetentOpen", SECTION_TYPE_NUMBER, &config.control.dropDetentOpen},
    {"rollLimit", SECTION_TYPE_NUMBER, &config.control.rollLimit},
    {"rollLimitHold", SECTION_TYPE_NUMBER, &config.control.rollLimitHold},
    {"pitchLowerLimit", SECTION_TYPE_NUMBER, &config.control.pitchLowerLimit},
    {"pitchUpperLimit", SECTION_TYPE_NUMBER, &config.control.pitchUpperLimit},
    {"maxAilDeflection", SECTION_TYPE_NUMBER, &config.control.maxAilDeflection},
    {"maxEleDeflection", SECTION_TYPE_NUMBER, &config.control.maxEleDeflection},
    {"maxRudDeflection", SECTION_TYPE_NUMBER, &config.control.maxRudDeflection},
    {"maxElevonDeflection", SECTION_TYPE_NUMBER, &config.control.maxElevonDeflection},
    {"elevonMixingGain", SECTION_TYPE_NUMBER, &config.control.elevonMixingGain},
    {"ailMixingBias", SECTION_TYPE_NUMBER, &config.control.ailMixingBias},
    {"elevMixingBias", SECTION_TYPE_NUMBER, &config.control.elevMixingBias},
};
static ConfigEntry configPins[] = {
    {"inputAil", SECTION_TYPE_NUMBER, &config.pins.inputAil},
    {"servoAil", SECTION_TYPE_NUMBER, &config.pins.servoAil},
    {"inputEle", SECTION_TYPE_NUMBER, &config.pins.inputEle},
    {"servoEle", SECTION_TYPE_NUMBER, &config.pins.servoEle},
    {"inputRud", SECTION_TYPE_NUMBER, &config.pins.inputRud},
    {"servoRud", SECTION_TYPE_NUMBER, &config.pins.servoRud},
    {"inputThrottle", SECTION_TYPE_NUMBER, &config.pins.inputThrottle},
    {"escThrottle", SECTION_TYPE_NUMBER, &config.pins.escThrottle},
    {"inputSwitch", SECTION_TYPE_NUMBER, &config.pins.inputSwitch},
    {"servoBay", SECTION_TYPE_NUMBER, &config.pins.servoBay},
    {"i2cSda", SECTION_TYPE_NUMBER, &config.pins.i2cSda},
    {"i2cScl", SECTION_TYPE_NUMBER, &config.pins.i2cScl},
    {"spiClk", SECTION_TYPE_NUMBER, &config.pins.spiClk},
    {"spiMosi", SECTION_TYPE_NUMBER, &config.pins.spiMosi},
    {"spiMiso", SECTION_TYPE_NUMBER, &config.pins.spiMiso},
    {"spiCs0", SECTION_TYPE_NUMBER, &config.pins.spiCs0},
    {"spiCs1", SECTION_TYPE_NUMBER, &config.pins.spiCs1},
    {"spiCs2", SECTION_TYPE_NUMBER, &config.pins.spiCs2},
    {"gpsTx", SECTION_TYPE_NUMBER, &config.pins.gpsTx},
    {"gpsRx", SECTION_TYPE_NUMBER, &config.pins.gpsRx},
    {"reverseRoll", SECTION_TYPE_NUMBER, &config.pins.reverseRoll},
    {"reversePitch", SECTION_TYPE_NUMBER, &config.pins.reversePitch},
    {"reverseYaw", SECTION_TYPE_NUMBER, &config.pins.reverseYaw},
};
static ConfigEntry configSensors[] = {
    {"busType", SECTION_TYPE_NUMBER, &config.sensors.busType},
    {"i2cBusFreq", SECTION_TYPE_NUMBER, &config.sensors.i2cBusFreq},
    {"spiBusFreq", SECTION_TYPE_NUMBER, &config.sensors.spiBusFreq},
    {"gpsCommandType", SECTION_TYPE_NUMBER, &config.sensors.gpsCommandType},
    {"gpsBaudrate", SECTION_TYPE_NUMBER, &config.sensors.gpsBaudrate},
};
static ConfigEntry configSystem[] = {
    {"ssid", SECTION_TYPE_STRING, config.system.ssid},
    {"pass", SECTION_TYPE_STRING, config.system.pass},
    {"print", SECTION_TYPE_NUMBER, &config.system.print},
    {"printIMU", SECTION_TYPE_NUMBER, &config.system.printIMU},
    {"printAircraft", SECTION_TYPE_NUMBER, &config.system.printAircraft},
    {"printGPS", SECTION_TYPE_NUMBER, &config.system.printGPS},
    {"printNetwork", SECTION_TYPE_NUMBER, &config.system.printNetwork},
};
static ConfigEntry configWebui[] = {
    {"defaultSpeed", SECTION_TYPE_STRING, &config.webui.defaultSpeed},
    {"dropSecs", SECTION_TYPE_STRING, &config.webui.dropSecs},
    {"pilotName", SECTION_TYPE_STRING, config.webui.pilotName},
    {"defaultMap", SECTION_TYPE_STRING, &config.webui.defaultMap},
    {"lastMapPosition", SECTION_TYPE_STRING, config.webui.lastMapPosition},
    {"lastMapZoom", SECTION_TYPE_STRING, config.webui.lastMapZoom},
    {"setupComplete", SECTION_TYPE_STRING, &config.webui.setupComplete},
};
static const ConfigSectionInfo configSections[] = {
    {CONFIG_GENERAL_STR, SECTION_TYPE_NUMBER, configGeneral, count_of(configGeneral)},
    {CONFIG_CONTROL_STR, SECTION_TYPE_NUMBER, configControl, count_of(configControl)},
    {CONFIG_PINS_STR, SECTION_TYPE_NUMBER, configPins, count_of(configPins)},
    {CONFIG_SENSORS_STR, SECTION_TYPE_NUMBER, configSensors, count_of(configSensors)},
    {CONFIG_SYSTEM_STR, SECTION_TYPE_NUMBER, configSystem, count_of(configSystem)},
    {CONFIG_WEBUI_STR, SECTION_TYPE_NONE, configWebui, count_of(configWebui)},
};

static ConfigEntry calibrationPWM[] = {
    {"calibrated", SECTION_TYPE_NUMBER, &calibration.pwm.calibrated},
    {"mode", SECTION_TYPE_NUMBER, &calibration.pwm.mode},
    {"offsetAil", SECTION_TYPE_NUMBER, &calibration.pwm.offsetAil},
    {"offsetEle", SECTION_TYPE_NUMBER, &calibration.pwm.offsetEle},
    {"offsetRud", SECTION_TYPE_NUMBER, &calibration.pwm.offsetRud},
    {"offsetSw", SECTION_TYPE_NUMBER, &calibration.pwm.offsetSw},
    {"offsetThr", SECTION_TYPE_NUMBER, &calibration.pwm.offsetThr},
};
static ConfigEntry calibrationESC[] = {
    {"calibrated", SECTION_TYPE_NUMBER, &calibration.esc.calibrated},
    {"idle", SECTION_TYPE_NUMBER, &calibration.esc.detentIdle},
    {"mct", SECTION_TYPE_NUMBER, &calibration.esc.detentMct},
    {"max", SECTION_TYPE_NUMBER, &calibration.esc.detentMax},
};
static ConfigEntry calibrationIMU[] = {
    {"calibrated", SECTION_TYPE_NUMBER, &calibration.imu.calibrated},
    {"gyroBiasX", SECTION_TYPE_NUMBER, &calibration.imu.gyroBiasX},
    {"gyroBiasY", SECTION_TYPE_NUMBER, &calibration.imu.gyroBiasY},
    {"gyroBiasZ", SECTION_TYPE_NUMBER, &calibration.imu.gyroBiasZ},
    {"accelOffsetX", SECTION_TYPE_NUMBER, &calibration.imu.accelOffsetX},
    {"accelOffsetY", SECTION_TYPE_NUMBER, &calibration.imu.accelOffsetY},
    {"accelOffsetZ", SECTION_TYPE_NUMBER, &calibration.imu.accelOffsetZ},
    {"axisMapRoll", SECTION_TYPE_NUMBER, &calibration.imu.axisMapRoll},
    {"axisMapPitch", SECTION_TYPE_NUMBER, &calibration.imu.axisMapPitch},
    {"axisMapYaw", SECTION_TYPE_NUMBER, &calibration.imu.axisMapYaw},
    {"axisSignRoll", SECTION_TYPE_NUMBER, &calibration.imu.axisSignRoll},
    {"axisSignPitch", SECTION_TYPE_NUMBER, &calibration.imu.axisSignPitch},
    {"axisSignYaw", SECTION_TYPE_NUMBER, &calibration.imu.axisSignYaw},
};
static ConfigEntry calibrationPID[] = {
    {"tuned", SECTION_TYPE_NUMBER, &calibration.pid.tuned},
    {"rollKp", SECTION_TYPE_NUMBER, &calibration.pid.rollKp},
    {"rollKi", SECTION_TYPE_NUMBER, &calibration.pid.rollKi},
    {"rollKd", SECTION_TYPE_NUMBER, &calibration.pid.rollKd},
    {"rollDb", SECTION_TYPE_NUMBER, &calibration.pid.rollDb},
    {"pitchKp", SECTION_TYPE_NUMBER, &calibration.pid.pitchKp},
    {"pitchKi", SECTION_TYPE_NUMBER, &calibration.pid.pitchKi},
    {"pitchKd", SECTION_TYPE_NUMBER, &calibration.pid.pitchKd},
    {"pitchDb", SECTION_TYPE_NUMBER, &calibration.pid.pitchDb},
    {"yawKp", SECTION_TYPE_NUMBER, &calibration.pid.yawKp},
    {"yawKi", SECTION_TYPE_NUMBER, &calibration.pid.yawKi},
    {"yawKd", SECTION_TYPE_NUMBER, &calibration.pid.yawKd},
    {"yawDb", SECTION_TYPE_NUMBER, &calibration.pid.yawDb},
    {"throttleKp", SECTION_TYPE_NUMBER, &calibration.pid.throttleKp},
    {"throttleKi", SECTION_TYPE_NUMBER, &calibration.pid.throttleKi},
    {"throttleKd", SECTION_TYPE_NUMBER, &calibration.pid.throttleKd},
    {"tau", SECTION_TYPE_NUMBER, &calibration.pid.tau},
};
static const ConfigSectionInfo calibrationSections[] = {
    {CONFIG_PWM_STR, SECTION_TYPE_NUMBER, calibrationPWM, count_of(calibrationPWM)},
    {CONFIG_ESC_STR, SECTION_TYPE_NUMBER, calibrationESC, count_of(calibrationESC)},
    {CONFIG_IMU_STR, SECTION_TYPE_NUMBER, calibrationIMU, count_of(calibrationIMU)},
    {CONFIG_PID_STR, SECTION_TYPE_NUMBER, calibrationPID, count_of(calibrationPID)},
};

// -- littlefs storage helpers --

/**
 * Writes a raw string buffer to a littlefs file, truncating any previous contents.
 * @param file the file name to write
 * @param contents the string contents to store
 * @return true if the write succeeded
 */
static bool write_file(const char *file, const char *contents) {
    lfs_file_t f;
    if (lfs_file_open(&lfs, &f, file, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC) != LFS_ERR_OK) {
        return false;
    }
    lfs_ssize_t expected = (lfs_ssize_t)strlen(contents);
    bool ok = lfs_file_write(&lfs, &f, contents, expected) == expected;
    if (ok) {
        ok = lfs_file_sync(&lfs, &f) == LFS_ERR_OK;
    }
    if (lfs_file_close(&lfs, &f) != LFS_ERR_OK) {
        ok = false;
    }
    return ok;
}

/**
 * Reads a littlefs file into a heap-allocated, null-terminated buffer.
 * @param file the file name to read
 * @param contents output pointer for the allocated buffer
 * @return true if the file was read successfully
 */
static bool read_file(const char *file, char **contents) {
    *contents = NULL;
    lfs_file_t f;
    if (lfs_file_open(&lfs, &f, file, LFS_O_RDONLY) != LFS_ERR_OK) {
        return false;
    }
    lfs_soff_t size = lfs_file_size(&lfs, &f);
    if (size < 0) {
        lfs_file_close(&lfs, &f);
        return false;
    }
    char *buffer = calloc((size_t)size + 1, sizeof(char));
    if (!buffer) {
        lfs_file_close(&lfs, &f);
        return false;
    }
    lfs_ssize_t read = lfs_file_read(&lfs, &f, buffer, (lfs_size_t)size);
    if (read != size) {
        free(buffer);
        lfs_file_close(&lfs, &f);
        return false;
    }
    if (lfs_file_close(&lfs, &f) != LFS_ERR_OK) {
        free(buffer);
        return false;
    }
    *contents = buffer;
    return true;
}

/**
 * Finds a config entry by key inside a section definition.
 * @param section the section metadata to search
 * @param key the entry name to look up
 * @return the matching entry, or NULL if it does not exist
 */
static const ConfigEntry *find_entry(const ConfigSectionInfo *section, const char *key) {
    if (!section) {
        return NULL;
    }
    for (size_t i = 0; i < section->entryCount; i++) {
        if (strcasecmp(section->entries[i].key, key) == 0) {
            return &section->entries[i];
        }
    }
    return NULL;
}

/**
 * Parses a finite floating point value from a string.
 * @param value the string to parse
 * @param out destination for the parsed number
 * @return true if parsing succeeded
 */
static bool parse_number(const char *value, f32 *out) {
    char *end = NULL;
    f32 parsed = strtof(value, &end);
    if (!end || end == value || *end != '\0' || !isfinite(parsed)) {
        return false;
    }
    *out = parsed;
    return true;
}

/**
 * Copies a string into a fixed-size destination (with bounds checking).
 * @param dest the destination buffer
 * @param dest_size the size of the destination buffer in bytes
 * @param value the string to copy
 * @return true if the string was copied and fits
 */
static bool copy_string(char *dest, size_t dest_size, const char *value) {
    if (strlen(value) >= dest_size) {
        return false;
    }
    snprintf(dest, dest_size, "%s", value);
    return true;
}

/**
 * Serializes a config-like section table to a JSON file.
 * @param file the target file name
 * @param version the schema version string to store
 * @param sections the section table to serialize
 * @param section_count number of sections in the table
 * @return true if the file was written successfully
 */
static bool write_json_storage(const char *file, const char *version, const ConfigSectionInfo *sections,
                               size_t section_count) {
    JSON_Value *root = json_value_init_object();
    if (!root) {
        return false;
    }
    JSON_Object *rootObj = json_value_get_object(root);

    // Write the current version to littlefs
    json_object_set_string(rootObj, "version", version);
    JSON_Value *sectionsVal = json_value_init_object();
    JSON_Object *sectionsObj = json_value_get_object(sectionsVal);

    // Now write all sections...
    for (size_t i = 0; i < section_count; i++) {
        const ConfigSectionInfo *section = &sections[i];
        JSON_Value *sectionVal = json_value_init_object();
        JSON_Object *sectionObj = json_value_get_object(sectionVal);
        // ...and each value of each section, depending on type
        for (size_t j = 0; j < section->entryCount; j++) {
            const ConfigEntry *entry = &section->entries[j];
            switch (entry->type) {
                case SECTION_TYPE_NUMBER:
                    json_object_set_number(sectionObj, entry->key, *(f32 *)entry->value);
                    break;
                case SECTION_TYPE_STRING:
                    json_object_set_string(sectionObj, entry->key, (const char *)entry->value);
                    break;
                default:
                    break;
            }
        }
        // Keep section names stable so existing saved files remain readable
        json_object_set_value(sectionsObj, section->name, sectionVal);
    }
    json_object_set_value(rootObj, "sections", sectionsVal);

    // Serialize and write to littlefs
    char *serialized = json_serialize_to_string(root);
    bool ok = serialized && write_file(file, serialized);
    json_free_serialized_string(serialized);
    json_value_free(root);
    return ok;
}

/**
 * Reads JSON (on-device) storage.
 * @param file the file name to load
 * @param version the expected schema version string
 * @param sections the section table to populate
 * @param section_count number of sections in the table
 * @return true if valid data was loaded or rewritten successfully
 * @note This function falls back to defaults when the requested file is missing or invalid.
 */
static bool read_json_storage(const char *file, const char *version, const ConfigSectionInfo *sections,
                              size_t section_count) {
    char *contents = NULL;
    if (!read_file(file, &contents)) {
        // File doesn't exist, create it and write default values which are present in the struct definition
        return write_json_storage(file, version, sections, section_count);
    }

    // Begin parsing the data we read
    JSON_Value *root = json_parse_string(contents);
    free(contents);
    if (!root) {
        return write_json_storage(file, version, sections, section_count);
    }

    // Check for a valid version and sections to continue our parse
    JSON_Object *rootObj = json_value_get_object(root);
    const char *fileVersion = rootObj ? json_object_get_string(rootObj, "version") : NULL;
    JSON_Object *sectionsObj = rootObj ? json_object_get_object(rootObj, "sections") : NULL;
    if (!rootObj || !sectionsObj || !fileVersion || strcmp(fileVersion, version) != 0) {
        json_value_free(root);
        return write_json_storage(file, version, sections, section_count);
    }

    // Recurse through sections and each section's entry to pull out each individual value (again dependant on type)
    for (size_t i = 0; i < section_count; i++) {
        const ConfigSectionInfo *section = &sections[i];
        JSON_Object *sectionObj = json_object_get_object(sectionsObj, section->name);
        if (!sectionObj) {
            continue;
        }
        for (size_t j = 0; j < section->entryCount; j++) {
            const ConfigEntry *entry = &section->entries[j];
            if (!json_object_has_value(sectionObj, entry->key)) {
                continue;
            }
            switch (entry->type) {
                case SECTION_TYPE_NUMBER: {
                    f32 parsed = 0;
                    if (json_object_has_value_of_type(sectionObj, entry->key, JSONNumber)) {
                        parsed = (f32)json_object_get_number(sectionObj, entry->key);
                    } else if (json_object_has_value_of_type(sectionObj, entry->key, JSONString)) {
                        const char *raw = json_object_get_string(sectionObj, entry->key);
                        if (!raw || !parse_number(raw, &parsed)) {
                            json_value_free(root);
                            return write_json_storage(file, version, sections, section_count);
                        }
                    } else {
                        json_value_free(root);
                        return write_json_storage(file, version, sections, section_count);
                    }
                    *(f32 *)entry->value = parsed;
                    break;
                }
                case SECTION_TYPE_STRING: {
                    const char *raw = json_object_get_string(sectionObj, entry->key);
                    if (!raw || !copy_string((char *)entry->value, CONFIG_STR_SIZE, raw)) {
                        json_value_free(root);
                        return write_json_storage(file, version, sections, section_count);
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }

    json_value_free(root);
    return true;
}

void config_load() {
    read_json_storage(FILE_CONFIG, CONFIG_VERSION, configSections, count_of(configSections));
    read_json_storage(FILE_CALIBRATION, CALIBRATION_VERSION, calibrationSections, count_of(calibrationSections));
    // Load print settings
    shouldPrint.fbw = config.system.print;
    shouldPrint.imu = config.system.printIMU;
    shouldPrint.aircraft = config.system.printAircraft;
    shouldPrint.gps = config.system.printGPS;
    shouldPrint.network = config.system.printNetwork;
}

void config_save() {
    write_json_storage(FILE_CONFIG, CONFIG_VERSION, configSections, count_of(configSections));
    write_json_storage(FILE_CALIBRATION, CALIBRATION_VERSION, calibrationSections, count_of(calibrationSections));
}

void config_reset() {
    lfs_remove(&lfs, FILE_CONFIG);
    lfs_remove(&lfs, FILE_CALIBRATION);
}

const ConfigSectionInfo *config_section_info(ConfigSection section) {
    if (section < 0 || section >= (ConfigSection)count_of(configSections)) {
        return NULL;
    }
    return &configSections[section];
}

const ConfigEntry *config_find_entry(const char *section, const char *key) {
    for (size_t i = 0; i < count_of(configSections); i++) {
        if (strcasecmp(configSections[i].name, section) == 0) {
            return find_entry(&configSections[i], key);
        }
    }
    return NULL;
}

ConfigSectionType config_get(const char *section, const char *key, void **value) {
    const ConfigEntry *entry = config_find_entry(section, key);
    if (!entry) {
        *value = NULL;
        return SECTION_TYPE_NONE;
    }
    *value = entry->value;
    return entry->type;
}

ConfigSetResult config_set(const char *section, const char *key, const char *value) {
    const ConfigEntry *entry = config_find_entry(section, key);
    if (!entry) {
        return CONFIG_SET_DOES_NOT_EXIST;
    }
    switch (entry->type) {
        case SECTION_TYPE_NUMBER: {
            f32 parsed = 0;
            if (!parse_number(value, &parsed)) {
                return CONFIG_SET_INVALID;
            }
            *(f32 *)entry->value = parsed;
            break;
        }
        case SECTION_TYPE_STRING:
            if (!copy_string((char *)entry->value, CONFIG_STR_SIZE, value)) {
                return CONFIG_SET_INVALID;
            }
            break;
        default:
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
