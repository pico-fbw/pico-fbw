/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/types.h"

#include "../io/aahrs.h"
#include "../io/flash.h"
#include "../io/gps.h"
#include "../io/pwm.h"

#include "config.h"

static void getFromGeneral(const char *key, float **value) {
    if (strcasecmp(key, "controlMode") == 0) {
        *value = &flash.general[GENERAL_CONTROL_MODE];
    } else if (strcasecmp(key, "switchType") == 0) {
        *value = &flash.general[GENERAL_SWITCH_TYPE];
    } else if (strcasecmp(key, "maxCalibrationOffset") == 0) {
        *value = &flash.general[GENERAL_MAX_CALIBRATION_OFFSET];
    } else if (strcasecmp(key, "servoHz") == 0) {
        *value = &flash.general[GENERAL_SERVO_HZ];
    } else if (strcasecmp(key, "escHz") == 0) {
        *value = &flash.general[GENERAL_ESC_HZ];
    } else if (strcasecmp(key, "apiEnabled") == 0) {
        *value = &flash.general[GENERAL_API_ENABLED];
    } else if (strcasecmp(key, "wiflyStatus") == 0) {
        *value = &flash.general[GENERAL_WIFLY_STATUS];
    } else if (strcasecmp(key, "skipCalibration") == 0) {
        *value = &flash.general[GENERAL_SKIP_CALIBRATION];
    } else {
        *value = NULL;
    }
}

static bool setToGeneral(const char *key, float value) {
    if (strcasecmp(key, "controlMode") == 0) {
        flash.general[GENERAL_CONTROL_MODE] = value;
    } else if (strcasecmp(key, "switchType") == 0) {
        flash.general[GENERAL_SWITCH_TYPE] = value;
    } else if (strcasecmp(key, "maxCalibrationOffset") == 0) {
        flash.general[GENERAL_MAX_CALIBRATION_OFFSET] = value;
    } else if (strcasecmp(key, "servoHz") == 0) {
        flash.general[GENERAL_SERVO_HZ] = value;
    } else if (strcasecmp(key, "escHz") == 0) {
        flash.general[GENERAL_ESC_HZ] = value;
    } else if (strcasecmp(key, "apiEnabled") == 0) {
        flash.general[GENERAL_API_ENABLED] = value;
    } else if (strcasecmp(key, "wiflyStatus") == 0) {
        flash.general[GENERAL_WIFLY_STATUS] = value;
    } else if (strcasecmp(key, "skipCalibration") == 0) {
        flash.general[GENERAL_SKIP_CALIBRATION] = value;
    } else return false;
    return true;
}

static void getFromControl(const char *key, float **value) {
    if (strcasecmp(key, "controlSensitivity") == 0) {
        *value = &flash.control[CONTROL_SENSITIVITY];
    } else if (strcasecmp(key, "rudderSensitivity") == 0) {
        *value = &flash.control[CONTROL_RUDDER_SENSITIVITY];
    } else if (strcasecmp(key, "controlDeadband") == 0) {
        *value = &flash.control[CONTROL_DEADBAND];
    } else if (strcasecmp(key, "throttleDetentsCalibrated") == 0) {
        *value = &flash.control[CONTROL_THROTTLE_DETENTS_CALIBRATED];
    } else if (strcasecmp(key, "throttleDetentIdle") == 0) {
        *value = &flash.control[CONTROL_THROTTLE_DETENT_IDLE];
    } else if (strcasecmp(key, "throttleDetentMCT") == 0) {
        *value = &flash.control[CONTROL_THROTTLE_DETENT_MCT];
    } else if (strcasecmp(key, "throttleDetentMax") == 0) {
        *value = &flash.control[CONTROL_THROTTLE_DETENT_MAX];
    } else if (strcasecmp(key, "throttleMaxTime") == 0) {
        *value = &flash.control[CONTROL_THROTTLE_MAX_TIME];
    } else if (strcasecmp(key, "throttleCooldownTime") == 0) {
        *value = &flash.control[CONTROL_THROTTLE_COOLDOWN_TIME];
    } else if (strcasecmp(key, "throttleSensitivity") == 0) {
        *value = &flash.control[CONTROL_THROTTLE_SENSITIVITY];
    } else if (strcasecmp(key, "dropDetentClosed") == 0) {
        *value = &flash.control[CONTROL_DROP_DETENT_CLOSED];
    } else if (strcasecmp(key, "dropDetentOpen") == 0) {
        *value = &flash.control[CONTROL_DROP_DETENT_OPEN];
    } else if (strcasecmp(key, "rollLimit") == 0) {
        *value = &flash.control[CONTROL_ROLL_LIMIT];
    } else if (strcasecmp(key, "rollLimitHold") == 0) {
        *value = &flash.control[CONTROL_ROLL_LIMIT_HOLD];
    } else if (strcasecmp(key, "pitchLowerLimit") == 0) {
        *value = &flash.control[CONTROL_PITCH_LOWER_LIMIT];
    } else if (strcasecmp(key, "pitchUpperLimit") == 0) {
        *value = &flash.control[CONTROL_PITCH_UPPER_LIMIT];
    } else if (strcasecmp(key, "maxAilDeflection") == 0) {
        *value = &flash.control[CONTROL_MAX_AIL_DEFLECTION];
    } else if (strcasecmp(key, "maxElevDeflection") == 0) {
        *value = &flash.control[CONTROL_MAX_ELEV_DEFLECTION];
    } else if (strcasecmp(key, "maxRudDeflection") == 0) {
        *value = &flash.control[CONTROL_MAX_RUD_DEFLECTION];
    } else if (strcasecmp(key, "maxElevonDeflection") == 0) {
        *value = &flash.control[CONTROL_MAX_ELEVON_DEFLECTION];
    } else if (strcasecmp(key, "elevonMixingGain") == 0) {
        *value = &flash.control[CONTROL_ELEVON_MIXING_GAIN];
    } else if (strcasecmp(key, "ailMixingBias") == 0) {
        *value = &flash.control[CONTROL_AIL_MIXING_BIAS];
    } else if (strcasecmp(key, "elevMixingBias") == 0) {
        *value = &flash.control[CONTROL_ELEV_MIXING_BIAS];
    } else {
        *value = NULL;
    }
}

static bool setToControl(const char *key, float value) {
    if (strcasecmp(key, "controlSensitivity") == 0) {
        flash.control[CONTROL_SENSITIVITY] = value;
    } else if (strcasecmp(key, "rudderSensitivity") == 0) {
        flash.control[CONTROL_RUDDER_SENSITIVITY] = value;
    } else if (strcasecmp(key, "controlDeadband") == 0) {
        flash.control[CONTROL_DEADBAND] = value;
    } else if (strcasecmp(key, "throttleDetentsCalibrated") == 0) {
        flash.control[CONTROL_THROTTLE_DETENTS_CALIBRATED] = value;
    } else if (strcasecmp(key, "throttleDetentIdle") == 0) {
        flash.control[CONTROL_THROTTLE_DETENT_IDLE] = value;
    } else if (strcasecmp(key, "throttleDetentMCT") == 0) {
        flash.control[CONTROL_THROTTLE_DETENT_MCT] = value;
    } else if (strcasecmp(key, "throttleDetentMax") == 0) {
        flash.control[CONTROL_THROTTLE_DETENT_MAX] = value;
    } else if (strcasecmp(key, "throttleMaxTime") == 0) {
        flash.control[CONTROL_THROTTLE_MAX_TIME] = value;
    } else if (strcasecmp(key, "throttleCooldownTime") == 0) {
        flash.control[CONTROL_THROTTLE_COOLDOWN_TIME] = value;
    } else if (strcasecmp(key, "throttleSensitivity") == 0) {
        flash.control[CONTROL_THROTTLE_SENSITIVITY] = value;
    } else if (strcasecmp(key, "dropDetentClosed") == 0) {
        flash.control[CONTROL_DROP_DETENT_CLOSED] = value;
    } else if (strcasecmp(key, "dropDetentOpen") == 0) {
        flash.control[CONTROL_DROP_DETENT_OPEN] = value;
    } else if (strcasecmp(key, "rollLimit") == 0) {
        flash.control[CONTROL_ROLL_LIMIT] = value;
    } else if (strcasecmp(key, "rollLimitHold") == 0) {
        flash.control[CONTROL_ROLL_LIMIT_HOLD] = value;
    } else if (strcasecmp(key, "pitchLowerLimit") == 0) {
        flash.control[CONTROL_PITCH_LOWER_LIMIT] = value;
    } else if (strcasecmp(key, "pitchUpperLimit") == 0) {
        flash.control[CONTROL_PITCH_UPPER_LIMIT] = value;
    } else if (strcasecmp(key, "maxAilDeflection") == 0) {
        flash.control[CONTROL_MAX_AIL_DEFLECTION] = value;
    } else if (strcasecmp(key, "maxElevDeflection") == 0) {
        flash.control[CONTROL_MAX_ELEV_DEFLECTION] = value;
    } else if (strcasecmp(key, "maxRudDeflection") == 0) {
        flash.control[CONTROL_MAX_RUD_DEFLECTION] = value;
    } else if (strcasecmp(key, "maxElevonDeflection") == 0) {
        flash.control[CONTROL_MAX_ELEVON_DEFLECTION] = value;
    } else if (strcasecmp(key, "elevonMixingGain") == 0) {
        flash.control[CONTROL_ELEVON_MIXING_GAIN] = value;
    } else if (strcasecmp(key, "ailMixingBias") == 0) {
        flash.control[CONTROL_AIL_MIXING_BIAS] = value;
    } else if (strcasecmp(key, "elevMixingBias") == 0) {
        flash.control[CONTROL_ELEV_MIXING_BIAS] = value;
    } else return false;
    return true;
}

static void getFromPins(const char *key, float **value) {
    if (strcasecmp(key, "inputAil") == 0) {
        *value = &flash.pins[PINS_INPUT_AIL];
    } else if (strcasecmp(key, "servoAil") == 0) {
        *value = &flash.pins[PINS_SERVO_AIL];
    } else if (strcasecmp(key, "inputElev") == 0) {
        *value = &flash.pins[PINS_INPUT_ELEV];
    } else if (strcasecmp(key, "servoElev") == 0) {
        *value = &flash.pins[PINS_SERVO_ELEV];
    } else if (strcasecmp(key, "inputRud") == 0) {
        *value = &flash.pins[PINS_INPUT_RUD];
    } else if (strcasecmp(key, "servoRud") == 0) {
        *value = &flash.pins[PINS_SERVO_RUD];
    } else if (strcasecmp(key, "inputThrottle") == 0) {
        *value = &flash.pins[PINS_INPUT_THROTTLE];
    } else if (strcasecmp(key, "escThrottle") == 0) {
        *value = &flash.pins[PINS_ESC_THROTTLE];
    } else if (strcasecmp(key, "inputSwitch") == 0) {
        *value = &flash.pins[PINS_INPUT_SWITCH];
    } else if (strcasecmp(key, "servoBay") == 0) {
        *value = &flash.pins[PINS_SERVO_BAY];
    } else if (strcasecmp(key, "aahrsSda") == 0) {
        *value = &flash.pins[PINS_AAHRS_SDA];
    } else if (strcasecmp(key, "aahrsScl") == 0) {
        *value = &flash.pins[PINS_AAHRS_SCL];
    } else if (strcasecmp(key, "gpsTx") == 0) {
        *value = &flash.pins[PINS_GPS_TX];
    } else if (strcasecmp(key, "gpsRx") == 0) {
        *value = &flash.pins[PINS_GPS_RX];
    } else if (strcasecmp(key, "reverseRoll") == 0) {
        *value = &flash.pins[PINS_REVERSE_ROLL];
    } else if (strcasecmp(key, "reversePitch") == 0) {
        *value = &flash.pins[PINS_REVERSE_PITCH];
    } else if (strcasecmp(key, "reverseYaw") == 0) {
        *value = &flash.pins[PINS_REVERSE_YAW];
    } else {
        *value = NULL;
    }
}

static bool setToPins(const char *key, float value) {
    if (strcasecmp(key, "inputAil") == 0) {
        flash.pins[PINS_INPUT_AIL] = value;
    } else if (strcasecmp(key, "servoAil") == 0) {
        flash.pins[PINS_SERVO_AIL] = value;
    } else if (strcasecmp(key, "inputElev") == 0) {
        flash.pins[PINS_INPUT_ELEV] = value;
    } else if (strcasecmp(key, "servoElev") == 0) {
        flash.pins[PINS_SERVO_ELEV] = value;
    } else if (strcasecmp(key, "inputRud") == 0) {
        flash.pins[PINS_INPUT_RUD] = value;
    } else if (strcasecmp(key, "servoRud") == 0) {
        flash.pins[PINS_SERVO_RUD] = value;
    } else if (strcasecmp(key, "inputThrottle") == 0) {
        flash.pins[PINS_INPUT_THROTTLE] = value;
    } else if (strcasecmp(key, "escThrottle") == 0) {
        flash.pins[PINS_ESC_THROTTLE] = value;
    } else if (strcasecmp(key, "inputSwitch") == 0) {
        flash.pins[PINS_INPUT_SWITCH] = value;
    } else if (strcasecmp(key, "servoBay") == 0) {
        flash.pins[PINS_SERVO_BAY] = value;
    } else if (strcasecmp(key, "aahrsSda") == 0) {
        flash.pins[PINS_AAHRS_SDA] = value;
    } else if (strcasecmp(key, "aahrsScl") == 0) {
        flash.pins[PINS_AAHRS_SCL] = value;
    } else if (strcasecmp(key, "gpsTx") == 0) {
        flash.pins[PINS_GPS_TX] = value;
    } else if (strcasecmp(key, "gpsRx") == 0) {
        flash.pins[PINS_GPS_RX] = value;
    } else if (strcasecmp(key, "reverseRoll") == 0) {
        flash.pins[PINS_REVERSE_ROLL] = value;
    } else if (strcasecmp(key, "reversePitch") == 0) {
        flash.pins[PINS_REVERSE_PITCH] = value;
    } else if (strcasecmp(key, "reverseYaw") == 0) {
        flash.pins[PINS_REVERSE_YAW] = value;
    } else return false;
    return true;
}

static void getFromSensors(const char *key, float **value) {
    if (strcasecmp(key, "imuModel") == 0) {
        *value = &flash.sensors[SENSORS_IMU_MODEL];
    } else if (strcasecmp(key, "baroModel") == 0) {
        *value = &flash.sensors[SENSORS_BARO_MODEL];
    } else if (strcasecmp(key, "gpsCommandType") == 0) {
        *value = &flash.sensors[SENSORS_GPS_COMMAND_TYPE];
    } else if (strcasecmp(key, "gpsBaudrate") == 0) {
        *value = &flash.sensors[SENSORS_GPS_BAUDRATE];
    } else {
        *value = NULL;
    }
}

static bool setToSensors(const char *key, float value) {
    if (strcasecmp(key, "imuModel") == 0) {
        flash.sensors[SENSORS_IMU_MODEL] = value;
    } else if (strcasecmp(key, "baroModel") == 0) {
        flash.sensors[SENSORS_BARO_MODEL] = value;
    } else if (strcasecmp(key, "gpsCommandType") == 0) {
        flash.sensors[SENSORS_GPS_COMMAND_TYPE] = value;
    } else if (strcasecmp(key, "gpsBaudrate") == 0) {
        flash.sensors[SENSORS_GPS_BAUDRATE] = value;
    } else return false;
    return true;
}

static void getFromSystem(const char *key, float **value) {
    if (strcasecmp(key, "debug") == 0) {
        *value = &flash.system[SYSTEM_DEBUG];
    } else if (strcasecmp(key, "debug_fbw") == 0) {
        *value = &flash.system[SYSTEM_DEBUG_FBW];
    } else if (strcasecmp(key, "debug_aahrs") == 0) {
        *value = &flash.system[SYSTEM_DEBUG_AAHRS];
    } else if (strcasecmp(key, "debug_gps") == 0) {
        *value = &flash.system[SYSTEM_DEBUG_GPS];
    } else if (strcasecmp(key, "debug_wifly") == 0) {
        *value = &flash.system[SYSTEM_DEBUG_WIFLY];
    } else if (strcasecmp(key, "debug_network") == 0) {
        *value = &flash.system[SYSTEM_DEBUG_NETWORK];
    } else if (strcasecmp(key, "dump_network") == 0) {
        *value = &flash.system[SYSTEM_DUMP_NETWORK];
    } else if (strcasecmp(key, "watchdogTimeout") == 0) {
        *value = &flash.system[SYSTEM_WATCHDOG_TIMEOUT];
    } else {
        *value = NULL;
    }
}

static bool setToSystem(const char *key, float value) {
    if (strcasecmp(key, "debug_fbw") == 0) {
        flash.system[SYSTEM_DEBUG_FBW] = value;
    } else if (strcasecmp(key, "debug_aahrs") == 0) {
        flash.system[SYSTEM_DEBUG_AAHRS] = value;
    } else if (strcasecmp(key, "debug_gps") == 0) {
        flash.system[SYSTEM_DEBUG_GPS] = value;
    } else if (strcasecmp(key, "debug_wifly") == 0) {
        flash.system[SYSTEM_DEBUG_WIFLY] = value;
    } else if (strcasecmp(key, "debug_network") == 0) {
        flash.system[SYSTEM_DEBUG_NETWORK] = value;
    } else if (strcasecmp(key, "dump_network") == 0) {
        flash.system[SYSTEM_DUMP_NETWORK] = value;
    } else if (strcasecmp(key, "watchdogTimeout") == 0) {
        flash.system[SYSTEM_WATCHDOG_TIMEOUT] = value;
    } else return false;
    return true;
}

static void getFromPID(const char *key, float **value) {
    if (strcasecmp(key, "tuneStatus") == 0) {
        *value = &flash.pid[PID_FLAG];
    } else if (strcasecmp(key, "roll_kp") == 0) {
        *value = &flash.pid[PID_ROLL_KP];
    } else if (strcasecmp(key, "roll_ki") == 0) {
        *value = &flash.pid[PID_ROLL_KI];
    } else if (strcasecmp(key, "roll_kd") == 0) {
        *value = &flash.pid[PID_ROLL_KD];
    } else if (strcasecmp(key, "roll_tau") == 0) {
        *value = &flash.pid[PID_ROLL_TAU];
    } else if (strcasecmp(key, "roll_integMin") == 0) {
        *value = &flash.pid[PID_ROLL_INTEGMIN];
    } else if (strcasecmp(key, "roll_integMax") == 0) {
        *value = &flash.pid[PID_ROLL_INTEGMAX];
    } else if (strcasecmp(key, "pitch_kp") == 0) {
        *value = &flash.pid[PID_PITCH_KP];
    } else if (strcasecmp(key, "pitch_ki") == 0) {
        *value = &flash.pid[PID_PITCH_KI];
    } else if (strcasecmp(key, "pitch_kd") == 0) {
        *value = &flash.pid[PID_PITCH_KD];
    } else if (strcasecmp(key, "pitch_tau") == 0) {
        *value = &flash.pid[PID_PITCH_TAU];
    } else if (strcasecmp(key, "pitch_integMin") == 0) {
        *value = &flash.pid[PID_PITCH_INTEGMIN];
    } else if (strcasecmp(key, "pitch_integMax") == 0) {
        *value = &flash.pid[PID_PITCH_INTEGMAX];
    } else if (strcasecmp(key, "yaw_kp") == 0) {
        *value = &flash.pid[PID_YAW_KP];
    } else if (strcasecmp(key, "yaw_ki") == 0) {
        *value = &flash.pid[PID_YAW_KI];
    } else if (strcasecmp(key, "yaw_kd") == 0) {
        *value = &flash.pid[PID_YAW_KD];
    } else if (strcasecmp(key, "yaw_tau") == 0) {
        *value = &flash.pid[PID_YAW_TAU];
    } else if (strcasecmp(key, "yaw_integMin") == 0) {
        *value = &flash.pid[PID_YAW_INTEGMIN];
    } else if (strcasecmp(key, "yaw_integMax") == 0) {
        *value = &flash.pid[PID_YAW_INTEGMAX];
    } else if (strcasecmp(key, "throttle_kp") == 0) {
        *value = &flash.pid[PID_THROTTLE_KP];
    } else if (strcasecmp(key, "throttle_ki") == 0) {
        *value = &flash.pid[PID_THROTTLE_KI];
    } else if (strcasecmp(key, "throttle_kd") == 0) {
        *value = &flash.pid[PID_THROTTLE_KD];
    } else if (strcasecmp(key, "throttle_tau") == 0) {
        *value = &flash.pid[PID_THROTTLE_TAU];
    } else if (strcasecmp(key, "throttle_integMin") == 0) {
        *value = &flash.pid[PID_THROTTLE_INTEGMIN];
    } else if (strcasecmp(key, "throttle_integMax") == 0) {
        *value = &flash.pid[PID_THROTTLE_INTEGMAX];
    } else {
        *value = NULL;
    }
}

static bool setToPID(const char *key, float value) {
    if (strcasecmp(key, "tuneStatus") == 0) {
        flash.pid[PID_FLAG] = value;
    } else if (strcasecmp(key, "roll_kp") == 0) {
        flash.pid[PID_ROLL_KP] = value;
    } else if (strcasecmp(key, "roll_ki") == 0) {
        flash.pid[PID_ROLL_KI] = value;
    } else if (strcasecmp(key, "roll_kd") == 0) {
        flash.pid[PID_ROLL_KD] = value;
    } else if (strcasecmp(key, "roll_tau") == 0) {
        flash.pid[PID_ROLL_TAU] = value;
    } else if (strcasecmp(key, "roll_integMin") == 0) {
        flash.pid[PID_ROLL_INTEGMIN] = value;
    } else if (strcasecmp(key, "roll_integMax") == 0) {
        flash.pid[PID_ROLL_INTEGMAX] = value;
    } else if (strcasecmp(key, "pitch_kp") == 0) {
        flash.pid[PID_PITCH_KP] = value;
    } else if (strcasecmp(key, "pitch_ki") == 0) {
        flash.pid[PID_PITCH_KI] = value;
    } else if (strcasecmp(key, "pitch_kd") == 0) {
        flash.pid[PID_PITCH_KD] = value;
    } else if (strcasecmp(key, "pitch_tau") == 0) {
        flash.pid[PID_PITCH_TAU] = value;
    } else if (strcasecmp(key, "pitch_integMin") == 0) {
        flash.pid[PID_PITCH_INTEGMIN] = value;
    } else if (strcasecmp(key, "pitch_integMax") == 0) {
        flash.pid[PID_PITCH_INTEGMAX] = value;
    } else if (strcasecmp(key, "yaw_kp") == 0) {
        flash.pid[PID_YAW_KP] = value;
    } else if (strcasecmp(key, "yaw_ki") == 0) {
        flash.pid[PID_YAW_KI] = value;
    } else if (strcasecmp(key, "yaw_kd") == 0) {
        flash.pid[PID_YAW_KD] = value;
    } else if (strcasecmp(key, "yaw_tau") == 0) {
        flash.pid[PID_YAW_TAU] = value;
    } else if (strcasecmp(key, "yaw_integMin") == 0) {
        flash.pid[PID_YAW_INTEGMIN] = value;
    } else if (strcasecmp(key, "yaw_integMax") == 0) {
        flash.pid[PID_YAW_INTEGMAX] = value;
    } else if (strcasecmp(key, "throttle_kp") == 0) {
        flash.pid[PID_THROTTLE_KP] = value;
    } else if (strcasecmp(key, "throttle_ki") == 0) {
        flash.pid[PID_THROTTLE_KI] = value;
    } else if (strcasecmp(key, "throttle_kd") == 0) {
        flash.pid[PID_THROTTLE_KD] = value;
    } else if (strcasecmp(key, "throttle_tau") == 0) {
        flash.pid[PID_THROTTLE_TAU] = value;
    } else if (strcasecmp(key, "throttle_integMin") == 0) {
        flash.pid[PID_THROTTLE_INTEGMIN] = value;
    } else if (strcasecmp(key, "throttle_integMax") == 0) {
        flash.pid[PID_THROTTLE_INTEGMAX] = value;
    } else return false;
    return true;
}

static void getFromWifly(const char *key, char **value) {
    if (strcasecmp(key, "ssid") == 0) {
        *value = flash.wifly_ssid;
    } else if (strcasecmp(key, "pass") == 0) {
        *value = flash.wifly_pass;
    } else {
        *value = NULL;
    }
}

static bool setToWifly(const char *key, const char *value) {
    if (strcasecmp(key, "ssid") == 0) {
        strcpy(flash.wifly_ssid, value);
    } else if (strcasecmp(key, "pass") == 0) {
        strcpy(flash.wifly_pass, value);
    } else return false;
    return true;
}

bool config_validate() {
    // Unique pin validation
    int lastPin = -1;
    switch ((ControlMode)flash.general[GENERAL_CONTROL_MODE]) {
        case CTRLMODE_3AXIS_ATHR:
            for (uint i = S_PIN_MIN; i <= S_PIN_MAX; i++) {
                if ((int)flash.pins[i] == lastPin) goto invalid;
            }
            break;
        case CTRLMODE_3AXIS:
            for (uint i = S_PIN_MIN; i <= S_PIN_MAX; i++) {
                // Skip pins that aren't utilized in this mode
                if (i == PINS_INPUT_THROTTLE || i == PINS_ESC_THROTTLE) break;
                if ((int)flash.pins[i] == lastPin) goto invalid;
            }
            break;
        case CTRLMODE_2AXIS_ATHR:
        case CTRLMODE_FLYINGWING_ATHR:
            for (uint i = S_PIN_MIN; i <= S_PIN_MAX; i++) {
                if (i == PINS_INPUT_RUD) break;
                if ((int)flash.pins[i] == lastPin) goto invalid;
            }
            break;
        case CTRLMODE_2AXIS:
        case CTRLMODE_FLYINGWING:
            for (uint i = S_PIN_MIN; i <= S_PIN_MAX; i++) {
                if (i == PINS_INPUT_RUD || i == PINS_INPUT_THROTTLE || i == PINS_ESC_THROTTLE) break;
                if ((int)flash.pins[i] == lastPin) goto invalid;
            }
            break;
        invalid:
            if (print.fbw) printf("ERROR: A pin may only be used once.\n");
            return false;
    }
    // Sensor pin validation
    // AAHRS_SDA can be on pins 0, 4, 8, 12, 16, 20, 28
    if ((uint)flash.pins[PINS_AAHRS_SDA] != 0 && (uint)flash.pins[PINS_AAHRS_SDA] != 4 &&(uint)flash.pins[PINS_AAHRS_SDA] != 8 &&
    (uint)flash.pins[PINS_AAHRS_SDA] != 12 && (uint)flash.pins[PINS_AAHRS_SDA] != 16 && (uint)flash.pins[PINS_AAHRS_SDA] != 20 &&
    (uint)flash.pins[PINS_AAHRS_SDA] != 28) {
        if (print.fbw) printf("ERROR: IMU_SDA must be on the I2C0_SDA interface.\n");
        return false;
    }
    // AAHRS_SCL can be on pins 1, 5, 9, 13, 17, 21
    if ((uint)flash.pins[PINS_AAHRS_SCL] != 1 && (uint)flash.pins[PINS_AAHRS_SCL] != 5 && (uint)flash.pins[PINS_AAHRS_SCL] != 9 &&
    (uint)flash.pins[PINS_AAHRS_SCL] != 13 && (uint)flash.pins[PINS_AAHRS_SCL] != 17 && (uint)flash.pins[PINS_AAHRS_SCL] != 21) {
        if (print.fbw) printf("ERROR: IMU_SCL must be on the I2C0_SCL interface.\n");
        return false;
    }
    // GPS_RX can be on pins 4, 8, 20
    if ((uint)flash.pins[PINS_GPS_RX] != 4 && (uint)flash.pins[PINS_GPS_RX] != 8 && (uint)flash.pins[PINS_GPS_RX] != 20) {
        if (print.fbw) printf("ERROR: GPS_RX must be on the UART1_RX interface.\n");
        return false;
    }
    // GPS_TX can be on pins 5, 9, 21
    if ((uint)flash.pins[PINS_GPS_TX] != 5 && (uint)flash.pins[PINS_GPS_TX] != 9 && (uint)flash.pins[PINS_GPS_TX] != 21) {
        if (print.fbw) printf("ERROR: GPS_TX must be on the UART1_TX interface.\n");
        return false;
    }
    // Limits validation
    if (flash.control[CONTROL_ROLL_LIMIT] > 72 || flash.control[CONTROL_ROLL_LIMIT] < 0) {
        if (print.fbw) printf("ERROR: Roll limit must be between 0 and 72 degrees.\n");
        return false;
    }
    if (flash.control[CONTROL_ROLL_LIMIT_HOLD] > 72 || flash.control[CONTROL_ROLL_LIMIT_HOLD] < 0) {
        if (print.fbw) printf("ERROR: Roll limit hold must be between 0 and 72 degrees.\n");
        return false;
    }
    if (flash.control[CONTROL_PITCH_UPPER_LIMIT] > 35 || flash.control[CONTROL_PITCH_UPPER_LIMIT] < 0) {
        if (print.fbw) printf("ERROR: Upper pitch limit must be between 0 and 35 degrees.\n");
        return false;
    }
    if (flash.control[CONTROL_PITCH_LOWER_LIMIT] < -20 || flash.control[CONTROL_PITCH_LOWER_LIMIT] > 0) {
        if (print.fbw) printf("ERROR: Lower pitch limit must be between -20 and 0 degrees.\n");
        return false;
    }
    // Sensor model validation
    if (flash.sensors[SENSORS_IMU_MODEL] < IMU_MODEL_MIN || flash.sensors[SENSORS_IMU_MODEL] > IMU_MODEL_MAX) {
        if (print.fbw) printf("ERROR: IMU model must be between %d and %d.\n", IMU_MODEL_MIN, IMU_MODEL_MAX);
        return false;
    }
    if (flash.sensors[SENSORS_BARO_MODEL] < BARO_MODEL_MIN || flash.sensors[SENSORS_BARO_MODEL] > BARO_MODEL_MAX) {
        if (print.fbw) printf("ERROR: Barometer model must be between %d and %d.\n", BARO_MODEL_MIN, BARO_MODEL_MAX);
        return false;
    }
    if (flash.sensors[SENSORS_GPS_COMMAND_TYPE] < GPS_COMMAND_TYPE_MIN || flash.sensors[SENSORS_GPS_COMMAND_TYPE] > GPS_COMMAND_TYPE_MAX) {
        if (print.fbw) printf("ERROR: GPS command type must be between %d and %d.\n", GPS_COMMAND_TYPE_MIN, GPS_COMMAND_TYPE_MAX);
        return false;
    }
    // Throttle detent and configuration validation
    if (flash.control[CONTROL_THROTTLE_DETENT_IDLE] < 0.0f || flash.control[CONTROL_THROTTLE_DETENT_IDLE] > 100.0f) {
        if (print.fbw) printf("ERROR: Idle throttle detent must be between 0 and 100 percent.\n");
        return false;
    }
    if (flash.control[CONTROL_THROTTLE_DETENT_MCT] < 0.0f || flash.control[CONTROL_THROTTLE_DETENT_MCT] > 100.0f) {
        if (print.fbw) printf("ERROR: MCT throttle detent must be between 0 and 100 percent.\n");
        return false;
    }
    if (flash.control[CONTROL_THROTTLE_DETENT_MAX] < 0.0f || flash.control[CONTROL_THROTTLE_DETENT_MAX] > 100.0f) {
        if (print.fbw) printf("ERROR: Max throttle detent must be between 0 and 100 percent.\n");
        return false;
    }
    if (flash.control[CONTROL_THROTTLE_SENSITIVITY] < 0.0f || flash.control[CONTROL_THROTTLE_SENSITIVITY] > 1.0f) {
        if (print.fbw) printf("ERROR: Throttle sensitivity must be between 0.0 and 1.0.\n");
        return false;
    }
    // Drop (servo position) validation
    if (flash.control[CONTROL_DROP_DETENT_CLOSED] < 0 || flash.control[CONTROL_DROP_DETENT_OPEN] > 180) {
        if (print.fbw) printf("ERROR: Drop detent (closed) must be between 0 and 180 degrees.\n");
        return false;
    }
    if (flash.control[CONTROL_DROP_DETENT_OPEN] < 0 || flash.control[CONTROL_DROP_DETENT_OPEN] > 180) {
        if (print.fbw) printf("ERROR: Drop detent (open) must be between 0 and 180 degrees.\n");
        return false;
    }
    // Control limit validation
    switch ((ControlMode)flash.general[GENERAL_CONTROL_MODE]) {
        case CTRLMODE_3AXIS_ATHR:
        case CTRLMODE_3AXIS:
            if (flash.control[CONTROL_MAX_AIL_DEFLECTION] > 90 || flash.control[CONTROL_MAX_AIL_DEFLECTION] < 0) {
                if (print.fbw) printf("ERROR: Max aileron deflection must be between 0 and 90 degrees.\n");
                return false;
            }
            if (flash.control[CONTROL_MAX_ELEV_DEFLECTION] > 90 || flash.control[CONTROL_MAX_ELEV_DEFLECTION] < 0) {
                if (print.fbw) printf("ERROR: Max elevator deflection must be between 0 and 90 degrees.\n");
                return false;
            }
            if (flash.control[CONTROL_MAX_RUD_DEFLECTION] > 90 || flash.control[CONTROL_MAX_RUD_DEFLECTION] < 0) {
                if (print.fbw) printf("ERROR: Max rudder deflection must be between 0 and 90 degrees.\n");
                return false;
            }
            break;
        case CTRLMODE_FLYINGWING_ATHR:
        case CTRLMODE_FLYINGWING:
            if (flash.control[CONTROL_MAX_ELEVON_DEFLECTION] > 90 || flash.control[CONTROL_MAX_ELEVON_DEFLECTION] < 0) {
                if (print.fbw) printf("ERROR: Max elevon deflection must be between 0 and 90 degrees.\n");
                return false;
            }
            break;
    }
    // Watchdog timeout validation
    if ((uint)flash.system[SYSTEM_WATCHDOG_TIMEOUT] < 1000 && (uint)flash.system[SYSTEM_WATCHDOG_TIMEOUT] > 0) {
        if (print.fbw) printf("ERROR: Watchdog timeout must be at least 1 second.\n");
        return false;
    }
    return true;
}

ConfigSectionType config_get(const char *section, const char *key, void **value) {
    if (strcasecmp(section, CONFIG_GENERAL_STR) == 0) {
        float *v;
        getFromGeneral(key, &v);
        *value = v;
        return SECTION_TYPE_FLOAT;
    } else if (strcasecmp(section, CONFIG_CONTROL_STR) == 0) {
        float *v;
        getFromControl(key, &v);
        *value = v;
        return SECTION_TYPE_FLOAT;
    } else if (strcasecmp(section, CONFIG_PINS_STR) == 0) {
        float *v;
        getFromPins(key, &v);
        *value = v;
        return SECTION_TYPE_FLOAT;
    } else if (strcasecmp(section, CONFIG_SENSORS_STR) == 0) {
        float *v;
        getFromSensors(key, &v);
        *value = v;
        return SECTION_TYPE_FLOAT;
    } else if (strcasecmp(section, CONFIG_WIFLY_STR) == 0) {
        char *v;
        getFromWifly(key, &v);
        *value = v;
        return SECTION_TYPE_STRING;
    } else if (strcasecmp(section, CONFIG_SYSTEM_STR) == 0) {
        float *v;
        getFromSystem(key, &v);
        *value = v;
        return SECTION_TYPE_FLOAT;
    } else if (strcasecmp(section, CONFIG_PID_STR) == 0) {
        float *v;
        getFromPID(key, &v);
        *value = v;
        return SECTION_TYPE_FLOAT;
    } else {
        return SECTION_TYPE_NONE;
    }
}

bool config_set(const char *section, const char *key, const char *value) {
    if (strcasecmp(section, CONFIG_GENERAL_STR) == 0) {
        if (!setToGeneral(key, atoff(value))) return false;
    } else if (strcasecmp(section, CONFIG_CONTROL_STR) == 0) {
        if (!setToControl(key, atoff(value))) return false;
    } else if (strcasecmp(section, CONFIG_PINS_STR) == 0) {
        if (!setToPins(key, atoff(value))) return false;
    } else if (strcasecmp(section, CONFIG_SENSORS_STR) == 0) {
        if (!setToSensors(key, atoff(value))) return false;
    } else if (strcasecmp(section, CONFIG_WIFLY_STR) == 0) {
        if (!setToWifly(key, value)) return false;
    } else if (strcasecmp(section, CONFIG_SYSTEM_STR) == 0) {
        if (!setToSystem(key, atoff(value))) return false;
    } else if (strcasecmp(section, CONFIG_PID_STR) == 0) {
        if (!setToPID(key, atoff(value))) return false;
    } else return false;
    return config_validate();
}

ConfigSectionType config_sectionToString(ConfigSection section, const char **str) {
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
        case CONFIG_WIFLY:
            *str = CONFIG_WIFLY_STR;
            return SECTION_TYPE_STRING;
        case CONFIG_SYSTEM:
            *str = CONFIG_SYSTEM_STR;
            return SECTION_TYPE_FLOAT;
        case CONFIG_PID:
            *str = CONFIG_PID_STR;
            return SECTION_TYPE_FLOAT;
    }
}
