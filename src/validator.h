#ifndef __VALIDATOR_H
#define __VALIDATOR_H

/* This file contains the validation of the config options. */

#if !defined(RASPBERRYPI_PICO) && !defined(RASPBERRYPI_PICO_W)
    #warning Neither a Pico or Pico W build target were found, some functionality may not work as intended.
#endif

/** @section general */

#if defined(CONTROL_3AXIS) && defined(CONTROL_FLYINGWING)
    #error Only one control method may be defined.
#endif

#if defined(SWITCH_2_POS) && defined(SWITCH_3_POS)
    #error Only one switch position may be defined.
#endif

/** @section control */

// Sadly the preprocessor doesn't support floating point comparisons so we can't check these

/** @section pins */

// Tried to make these variadic but it didn't work so we have to have this mess :(
#define PINS_UNIQUE_13(a, b, c, d, e, f, g, h, i, j, k, l, m) \
    ((a != b) && (a != c) && (a != d) && (a != e) && (a != f) && (a != g) && (a != h) && (a != i) && (a != j) && (a != k) && (a != l) && (a != m) && \
     (b != c) && (b != d) && (b != e) && (b != f) && (b != g) && (b != h) && (b != i) && (b != j) && (b != k) && (b != l) && (b != m) && \
     (c != d) && (c != e) && (c != f) && (c != g) && (c != h) && (c != i) && (c != j) && (c != k) && (c != l) && (c != m) && \
     (d != e) && (d != f) && (d != g) && (d != h) && (d != i) && (d != j) && (d != k) && (d != l) && (d != m) && \
     (e != f) && (e != g) && (e != h) && (e != i) && (e != j) && (e != k) && (e != l) && (e != m) && \
     (f != g) && (f != h) && (f != i) && (f != j) && (f != k) && (f != l) && (f != m) && \
     (g != h) && (g != i) && (g != j) && (g != k) && (g != l) && (g != m) && \
     (h != i) && (h != j) && (h != k) && (h != l) && (h != m) && \
     (i != j) && (i != k) && (i != l) && (i != m) && \
     (j != k) && (j != l) && (j != m) && \
     (k != l) && (k != m) && \
     (l != m))
#define PINS_UNIQUE_11(a, b, c, d, e, f, g, h, i, j, k) \
    ((a != b) && (a != c) && (a != d) && (a != e) && (a != f) && (a != g) && (a != h) && (a != i) && (a != j) && (a != k) && \
     (b != c) && (b != d) && (b != e) && (b != f) && (b != g) && (b != h) && (b != i) && (b != j) && (b != k) && \
     (c != d) && (c != e) && (c != f) && (c != g) && (c != h) && (c != i) && (c != j) && (c != k) && \
     (d != e) && (d != f) && (d != g) && (d != h) && (d != i) && (d != j) && (d != k) && \
     (e != f) && (e != g) && (e != h) && (e != i) && (e != j) && (e != k) && \
     (f != g) && (f != h) && (f != i) && (f != j) && (f != k) && \
     (g != h) && (g != i) && (g != j) && (g != k) && \
     (h != i) && (h != j) && (h != k) && \
     (i != j) && (i != k) && \
     (j != k))

#if defined(CONTROL_3AXIS)
    #if !PINS_UNIQUE_13(INPUT_AIL_PIN, SERVO_AIL_PIN, INPUT_ELEV_PIN, SERVO_ELEV_PIN, INPUT_RUD_PIN, SERVO_RUD_PIN, INPUT_THR_PIN, ESC_THR_PIN, INPUT_SW_PIN, IMU_SDA_PIN, IMU_SCL_PIN, GPS_RX_PIN, GPS_TX_PIN)
        #error A pin may only be assigned once.
    #endif
#elif defined(CONTROL_FLYINGWING)
    #if !PINS_UNIQUE_11(INPUT_AIL_PIN, SERVO_ELEVON_L_PIN, INPUT_ELEV_PIN, SERVO_ELEVON_R_PIN, INPUT_THR_PIN, ESC_THR_PIN, INPUT_SW_PIN, IMU_SDA_PIN, IMU_SCL_PIN, GPS_RX_PIN, GPS_TX_PIN)
        #error A pin may only be assigned once.
    #endif
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

#if (MAX_AIL_DEFLECTION > 90 || MAX_ELEV_DEFLECTION > 90 || MAX_RUD_DEFLECTION > 90)
    #error Aileron, elevator, and rudder servo limit(s) must be less than 90 degrees.
#endif

/** @section sensors */

#if defined(IMU_BNO055) && defined(IMU_MPU6050)
    #error Only one IMU module may be defined.
#endif
#if !defined(IMU_BNO055) && !defined(IMU_MPU6050)
    #error An IMU module must be defined.
#endif

#if (IMU_SDA_PIN != 0 && IMU_SDA_PIN != 4 && IMU_SDA_PIN != 8 && IMU_SDA_PIN != 12 && IMU_SDA_PIN != 16 && IMU_SDA_PIN != 20 && IMU_SDA_PIN != 28)
    #error IMU_SDA_PIN must be on the I2C0_SDA interface.
    #undef IMU_SDA_PIN
#endif
#if (IMU_SCL_PIN != 1 && IMU_SCL_PIN != 5 && IMU_SCL_PIN != 9 && IMU_SCL_PIN != 13 && IMU_SCL_PIN != 17 && IMU_SCL_PIN != 21)
    #error IMU_SCL_PIN must be on the I2C0_SCL interface.
    #undef IMU_SCL_PIN
#endif

#if (GPS_RX_PIN != 4 && GPS_RX_PIN != 8 && GPS_RX_PIN != 20)
    #error GPS_RX_PIN must be on the UART1_TX interface.
    #undef GPS_RX_PIN
#endif
#if (GPS_TX_PIN != 5 && GPS_TX_PIN != 9 && GPS_TX_PIN != 21)
    #error GPS_TX_PIN must be on the UART1_RX interface.
    #undef GPS_TX_PIN
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