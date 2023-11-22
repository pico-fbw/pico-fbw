#ifndef __FLASH_H
#define __FLASH_H

/**
 * README:
 * This file is of paramount importance. Please do NOT change anything in here unless you REALLY know what you are doing!
 * Many values here change where the system looks for flash values, which can cause data corruption/erasure.
 * Be warned!!
*/

#include "hardware/flash.h"

#include "aahrs.h"
#include "gps.h"
#include "pwm.h"
#include "../lib/fusion/calibration.h"
#include "../sys/switch.h"
#include "../wifly/wifly.h"

typedef enum SectorCalibration {
    CALIBRATION_BOOT_FLAG,
    #define FLAG_BOOT 3.1305210f
    CALIBRATION_PWM_CALIBRATED,
    CALIBRATION_PWM_MODE,
    CALIBRATION_PWM_OFFSET_AIL,
    CALIBRATION_PWM_OFFSET_ELEV,
    CALIBRATION_PWM_OFFSET_RUD,
    CALIBRATION_PWM_OFFSET_SW,
    CALIBRATION_PWM_OFFSET_THR,
    CALIBRATION_AAHRS_CALIBRATED,
    CALIBRATION_AAHRS_IMU_MODEL,
    CALIBRATION_AAHRS_BARO_MODEL
} SectorCalibration;
#define S_CALIBRATION_HIGHEST CALIBRATION_AAHRS_CALIBRATED

typedef enum SectorPID {
    PID_FLAG,

    PID_ROLL_KP,
    PID_ROLL_TI,
    PID_ROLL_TD,
    PID_ROLL_KT,
    PID_ROLL_TAU,
    PID_ROLL_INTEGMIN,
    PID_ROLL_INTEGMAX,

    PID_PITCH_KP,
    PID_PITCH_TI,
    PID_PITCH_TD,
    PID_PITCH_KT,
    PID_PITCH_TAU,
    PID_PITCH_INTEGMIN,
    PID_PITCH_INTEGMAX,

    PID_YAW_KP,
    PID_YAW_TI,
    PID_YAW_TD,
    PID_YAW_KT,
    PID_YAW_TAU,
    PID_YAW_INTEGMIN,
    PID_YAW_INTEGMAX
} SectorPID;
#define S_PID_HIGHEST PID_YAW_INTEGMAX

typedef enum SectorConfigGeneral {
    GENERAL_CONTROL_MODE,
    GENERAL_SWITCH_TYPE,
    GENERAL_MAX_CALIBRATION_OFFSET,
    GENERAL_SERVO_HZ,
    GENERAL_ESC_HZ,
    GENERAL_API_ENABLED,
    GENERAL_WIFLY_STATUS,
    GENERAL_SKIP_CALIBRATION
} SectorConfigGeneral;
#define S_GENERAL_HIGHEST GENERAL_SKIP_CALIBRATION

typedef enum SectorConfigControl {
    CONTROL_SENSITIVITY,
    CONTROL_RUDDER_SENSITIVITY,
    CONTROL_DEADBAND,

    CONTROL_THROTTLE_DETENTS_CALIBRATED,
    CONTROL_THROTTLE_DETENT_IDLE,
    CONTROL_THROTTLE_DETENT_MCT,
    CONTROL_THROTTLE_DETENT_MAX,
    CONTROL_THROTTLE_MAX_TIME,

    CONTROL_DROP_DETENT_CLOSED,
    CONTROL_DROP_DETENT_OPEN,

    CONTROL_ROLL_LIMIT,
    CONTROL_ROLL_LIMIT_HOLD,
    CONTROL_PITCH_LOWER_LIMIT,
    CONTROL_PITCH_UPPER_LIMIT,

    CONTROL_MAX_AIL_DEFLECTION,
    CONTROL_MAX_ELEV_DEFLECTION,
    CONTROL_MAX_RUD_DEFLECTION,

    CONTROL_MAX_ELEVON_DEFLECTION,
    CONTROL_ELEVON_MIXING_GAIN,
    CONTROL_AIL_MIXING_BIAS,
    CONTROL_ELEV_MIXING_BIAS
} SectorConfigControl;
#define S_CONTROL_HIGHEST CONTROL_ELEV_MIXING_BIAS

typedef enum SectorConfigPins {
    PINS_INPUT_AIL,
    PINS_SERVO_AIL,
    PINS_INPUT_ELEV,
    PINS_SERVO_ELEV,
    PINS_INPUT_RUD,
    PINS_SERVO_RUD,
    PINS_INPUT_THROTTLE,
    PINS_ESC_THROTTLE,
    PINS_INPUT_SWITCH,
    PINS_SERVO_DROP,
    PINS_SERVO_ELEVON_L,
    PINS_SERVO_ELEVON_R,

    PINS_AAHRS_SDA,
    PINS_AAHRS_SCL,
    PINS_GPS_TX,
    PINS_GPS_RX,

    PINS_REVERSE_ROLL,
    PINS_REVERSE_PITCH,
    PINS_REVERSE_YAW
} SectorConfigPins;
#define S_PIN_MIN PINS_INPUT_AIL
#define S_PIN_MAX PINS_GPS_RX
#define S_PINS_HIGHEST PINS_REVERSE_YAW

typedef enum SectorConfigSensors {
    SENSORS_IMU_MODEL,
    SENSORS_BARO_MODEL,
    SENSORS_GPS_COMMAND_TYPE,
    SENSORS_GPS_BAUDRATE,
} SectorConfigSensors;
#define S_SENSORS_HIGHEST SENSORS_GPS_BAUDRATE

typedef enum SectorConfigSystem {
    SYSTEM_DEBUG,
    SYSTEM_DEBUG_FBW,
    SYSTEM_DEBUG_AAHRS,
    SYSTEM_DEBUG_GPS,
    SYSTEM_DEBUG_WIFLY,
    SYSTEM_DEBUG_NETWORK,
    SYSTEM_DUMP_NETWORK,
    SYSTEM_WATCHDOG_TIMEOUT
} SectorConfigSystem;
#define S_SYSTEM_HIGHEST SYSTEM_WATCHDOG_TIMEOUT

// Total number of float sectors
#define FLASH_NUM_FLOAT_SECTORS 32
// The amount of floats that can be fit in one flash page (don't use 1024?)
#define FLOAT_SECTOR_SIZE_FULL FLASH_SECTOR_SIZE/sizeof(float)
// The amount of floats that can be fit in one float sector
#define FLOAT_SECTOR_SIZE FLOAT_SECTOR_SIZE_FULL/FLASH_NUM_FLOAT_SECTORS
#define FLOAT_SECTOR_SIZE_BYTES FLOAT_SECTOR_SIZE*sizeof(float)
// The physical sector that the virtual float "sectors" are placed in
#define FLOAT_PHYSECTOR 0 // Sector zero is the last sector on the flash memory (eg. the top of the 2MB flash) so it is far away from program data

// Total number of string sectors
#define FLASH_NUM_STRING_SECTORS 32
// The amount of chars that can be fit in one flash page
#define STRING_SECTOR_SIZE_FULL FLASH_SECTOR_SIZE/sizeof(char)
// The amount of chars that can be fit in one string sector
#define STRING_SECTOR_SIZE STRING_SECTOR_SIZE_FULL/FLASH_NUM_STRING_SECTORS
#define STRING_SECTOR_SIZE_BYTES STRING_SECTOR_SIZE*sizeof(char)
// The physical sector that the virtual string "sectors" are placed in
#define STRING_PHYSECTOR 1

// Signifies the end of a flash sector if it doesn't take up the entire allotted memory
// Used mostly for the GET_CONFIG API command, pico-fbw itself only really cares about the locations of data
#define FLAG_END (-30.54245f)

typedef struct Flash {
    /* Auto-generated system settings/calibrations */
    float calibration[FLOAT_SECTOR_SIZE];
    float pid[FLOAT_SECTOR_SIZE];
    float aahrs[FUSION_CALIBRATION_STORAGE_SIZE];
    /* User-defined config */
    float general[FLOAT_SECTOR_SIZE];
    float control[FLOAT_SECTOR_SIZE];
    float pins[FLOAT_SECTOR_SIZE];
    float sensors[FLOAT_SECTOR_SIZE];
    float system[FLOAT_SECTOR_SIZE];

    char version[STRING_SECTOR_SIZE];
    char wifly_ssid[STRING_SECTOR_SIZE];
    char wifly_pass[STRING_SECTOR_SIZE];
} Flash;
#define NUM_FLOAT_SECTORS (7 + (FUSION_CALIBRATION_STORAGE_SIZE / (FLOAT_SECTOR_SIZE)))
#define NUM_STRING_SECTORS 3

#define SIZEOF_FLOAT_SECTORS (NUM_FLOAT_SECTORS * FLOAT_SECTOR_SIZE)
#define SIZEOF_FLOAT_SECTORS_BYTES (NUM_FLOAT_SECTORS * FLOAT_SECTOR_SIZE_BYTES)
#define SIZEOF_STRING_SECTORS (NUM_STRING_SECTORS * STRING_SECTOR_SIZE)
#define SIZEOF_STRING_SECTORS_BYTES (NUM_STRING_SECTORS * STRING_SECTOR_SIZE_BYTES)

typedef struct PrintDefs {
    bool fbw, aahrs, gps, wifly, network, dumpNetwork;
} PrintDefs;

extern Flash flash;
extern PrintDefs print;

// Gets the memory location IN FLASH of a given PHYSICAL (not virtual!) sector.
#define GET_PHYSECTOR_LOC(sector) (PICO_FLASH_SIZE_BYTES - (FLASH_SECTOR_SIZE * (sector + 1)))
// Gets the absolute memory location of a given PHYSICAL (not virtual!) sector.
#define GET_PHYSECTOR_LOC_ABSOLUTE(sector) (XIP_BASE + (GET_PHYSECTOR_LOC(sector)))

/**
 * This will erase ALL flash sectors used by pico-fbw, destroying ANY config/calibration data!
 * @note It will also put the physical sectors into a writable state.
*/
void flash_erase();

/**
 * Loads the current content from flash into the Flash struct.
 * @return The number of bytes read from flash
 * @note This function will initialize flash if necessary.
*/
uint flash_load();

/**
 * Saves the current content of the Flash struct to flash.
*/
void flash_save();

#endif // __FLASH_H
