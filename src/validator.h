#ifndef __VALIDATOR_H
#define __VALIDATOR_H

/* This file contains the validation of the config options */

#include "config.h"

#if !defined(RASPBERRYPI_PICO) && !defined(RASPBERRYPI_PICO_W)
    #warning Neither a Pico or Pico W build target were found, some functionality may not work as intended.
#endif

/** @section control */

// Sadly the preprocessor doesn't support floating point comparisons so we can't check these

/** @section general */

#if defined(SWITCH_2_POS) && defined(SWITCH_3_POS)
    #error Only one switch position may be defined.
#endif

/** @section pins */

#define PINS_EQUAL(p1, p2) ((p1) == (p2))

#if PINS_EQUAL(INPUT_AIL_PIN, SERVO_AIL_PIN) || \
    PINS_EQUAL(INPUT_ELEV_PIN, SERVO_ELEV_PIN) || \
    PINS_EQUAL(INPUT_RUD_PIN, SERVO_RUD_PIN) || \
    PINS_EQUAL(INPUT_AIL_PIN, INPUT_SW_PIN) || \
    PINS_EQUAL(INPUT_AIL_PIN, INPUT_THR_PIN) || \
    PINS_EQUAL(INPUT_AIL_PIN, IMU_SDA_PIN) || \
    PINS_EQUAL(INPUT_AIL_PIN, IMU_SCL_PIN) || \
    PINS_EQUAL(INPUT_AIL_PIN, GPS_TX_PIN) || \
    PINS_EQUAL(INPUT_AIL_PIN, GPS_RX_PIN) || \
    PINS_EQUAL(SERVO_AIL_PIN, INPUT_SW_PIN) || \
    PINS_EQUAL(SERVO_AIL_PIN, INPUT_THR_PIN) || \
    PINS_EQUAL(SERVO_AIL_PIN, IMU_SDA_PIN) || \
    PINS_EQUAL(SERVO_AIL_PIN, IMU_SCL_PIN) || \
    PINS_EQUAL(SERVO_AIL_PIN, GPS_TX_PIN) || \
    PINS_EQUAL(SERVO_AIL_PIN, GPS_RX_PIN) || \
    PINS_EQUAL(INPUT_ELEV_PIN, INPUT_SW_PIN) || \
    PINS_EQUAL(INPUT_ELEV_PIN, INPUT_THR_PIN) || \
    PINS_EQUAL(INPUT_ELEV_PIN, IMU_SDA_PIN) || \
    PINS_EQUAL(INPUT_ELEV_PIN, IMU_SCL_PIN) || \
    PINS_EQUAL(INPUT_ELEV_PIN, GPS_TX_PIN) || \
    PINS_EQUAL(INPUT_ELEV_PIN, GPS_RX_PIN) || \
    PINS_EQUAL(SERVO_ELEV_PIN, INPUT_SW_PIN) || \
    PINS_EQUAL(SERVO_ELEV_PIN, INPUT_THR_PIN) || \
    PINS_EQUAL(SERVO_ELEV_PIN, IMU_SDA_PIN) || \
    PINS_EQUAL(SERVO_ELEV_PIN, IMU_SCL_PIN) || \
    PINS_EQUAL(SERVO_ELEV_PIN, GPS_TX_PIN) || \
    PINS_EQUAL(SERVO_ELEV_PIN, GPS_RX_PIN) || \
    PINS_EQUAL(INPUT_RUD_PIN, INPUT_SW_PIN) || \
    PINS_EQUAL(INPUT_RUD_PIN, INPUT_THR_PIN) || \
    PINS_EQUAL(INPUT_RUD_PIN, IMU_SDA_PIN) || \
    PINS_EQUAL(INPUT_RUD_PIN, IMU_SCL_PIN) || \
    PINS_EQUAL(INPUT_RUD_PIN, GPS_TX_PIN) || \
    PINS_EQUAL(INPUT_RUD_PIN, GPS_RX_PIN) || \
    PINS_EQUAL(SERVO_RUD_PIN, INPUT_SW_PIN) || \
    PINS_EQUAL(SERVO_RUD_PIN, INPUT_THR_PIN) || \
    PINS_EQUAL(SERVO_RUD_PIN, IMU_SDA_PIN) || \
    PINS_EQUAL(SERVO_RUD_PIN, IMU_SCL_PIN) || \
    PINS_EQUAL(SERVO_RUD_PIN, GPS_TX_PIN) || \
    PINS_EQUAL(SERVO_RUD_PIN, GPS_RX_PIN) || \
    PINS_EQUAL(INPUT_SW_PIN, IMU_SDA_PIN) || \
    PINS_EQUAL(INPUT_SW_PIN, IMU_SCL_PIN) || \
    PINS_EQUAL(INPUT_SW_PIN, GPS_TX_PIN) || \
    PINS_EQUAL(INPUT_SW_PIN, GPS_RX_PIN) || \
    PINS_EQUAL(INPUT_THR_PIN, IMU_SDA_PIN) || \
    PINS_EQUAL(INPUT_THR_PIN, IMU_SCL_PIN) || \
    PINS_EQUAL(INPUT_THR_PIN, GPS_TX_PIN) || \
    PINS_EQUAL(INPUT_THR_PIN, GPS_RX_PIN) || \
    PINS_EQUAL(IMU_SDA_PIN, IMU_SCL_PIN) || \
    PINS_EQUAL(IMU_SDA_PIN, GPS_TX_PIN) || \
    PINS_EQUAL(IMU_SDA_PIN, GPS_RX_PIN) || \
    PINS_EQUAL(IMU_SCL_PIN, GPS_TX_PIN) || \
    PINS_EQUAL(IMU_SCL_PIN, GPS_RX_PIN) || \
    PINS_EQUAL(GPS_TX_PIN, GPS_RX_PIN)
    #error Two or more pins are the same. Pins must be unique.
#endif

/** @section limits */

#if (ROLL_LIMIT > 72 || ROLL_LIMIT_HOLD > 72)
    #error Roll limit(s) must be less than 72 degrees.
#endif
#if (PITCH_UPPER_LIMIT > 35)
    #error The pitch upper limit must be less than 35 degrees.
#endif
#if (PITCH_LOWER_LIMIT < -20)
    #error The pitch lower limit must be greater than -20 degrees.
#endif

#if (AIL_LIMIT > 90 || ELEV_LIMIT > 90 || RUD_LIMIT > 90)
    #error Aileron, elevator, and rudder servo limit(s) must be less than 90 degrees.
#endif

/** @section sensors */

#if defined(IMU_BNO055) && defined(IMU_MPU6050)
    #error Only one IMU module may be defined.
#endif
#if !defined(IMU_BNO055) && !defined(IMU_MPU6050)
    #error An IMU module must be defined.
#endif

#if (IMU_X_AXIS != ROLL_AXIS && IMU_X_AXIS != PITCH_AXIS && IMU_X_AXIS != YAW_AXIS && IMU_X_AXIS != IMU_Y_AXIS && IMU_X_AXIS != IMU_Z_AXIS)
	#error IMU_X_AXIS must be either ROLL_AXIS, PITCH_AXIS, or YAW_AXIS.
	#undef IMU_X_AXIS
#endif
#if (IMU_Y_AXIS != ROLL_AXIS && IMU_Y_AXIS != PITCH_AXIS && IMU_Y_AXIS != YAW_AXIS && IMU_Y_AXIS != IMU_X_AXIS && IMU_Y_AXIS != IMU_Z_AXIS)
	#error IMU_Y_AXIS must be either ROLL_AXIS, PITCH_AXIS, or YAW_AXIS.
	#undef IMU_Y_AXIS
#endif
#if (IMU_Z_AXIS != ROLL_AXIS && IMU_Z_AXIS != PITCH_AXIS && IMU_Z_AXIS != YAW_AXIS && IMU_Z_AXIS != IMU_X_AXIS && IMU_Z_AXIS != IMU_Y_AXIS)
	#error IMU_Z_AXIS must be either ROLL_AXIS, PITCH_AXIS, or YAW_AXIS.
	#undef IMU_Z_AXIS
#endif

#if !defined(GPS_COMMAND_TYPE_PMTK) && !defined(GPS_COMMAND_TYPE_PSRF)
    #error A GPS command type must be defined.
#endif

/** @section Wi-Fly */

#ifdef GPS_ENABLED
    #if !defined(API_ENABLED) && !defined(WIFLY_ENABLED)
        #error Neither the API nor Wi-Fly (Pico W required) were enabled, there is no way to upload a flightplan for auto mode! Please either enable one of these options or disable GPS to continue.
    #endif
#endif

/** @section debug/api */

#if defined(LIB_PICO_STDIO_USB) || defined(LIB_PICO_STDIO_UART)
    #ifndef FBW_DEBUG
        #error FBW_DEBUG must be defined if LIB_PICO_STDIO_USB or LIB_PICO_STDIO_UART is defined.
    #endif
#endif

#endif // __VALIDATOR_H