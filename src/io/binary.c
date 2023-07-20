#include "pico/binary_info.h"

#include "hardware/gpio.h"

#include "../config.h"

/* This file compiles all pin definitions into the binary. */

// GPS communication pins (UART)
bi_decl(bi_2pins_with_func(GPS_TX_PIN, GPS_RX_PIN, GPIO_FUNC_UART));

// IMU communication pins (I2C)
bi_decl(bi_2pins_with_func(IMU_SDA_PIN, IMU_SCL_PIN, GPIO_FUNC_I2C));

// PWM input pins
bi_decl(bi_4pins_with_func(INPUT_AIL_PIN, INPUT_ELEV_PIN, INPUT_RUD_PIN, MODE_SWITCH_PIN, GPIO_FUNC_PIO0));

// PWM output (servo) pins
bi_decl(bi_3pins_with_func(SERVO_AIL_PIN, SERVO_ELEV_PIN, SERVO_RUD_PIN, GPIO_FUNC_PWM));
