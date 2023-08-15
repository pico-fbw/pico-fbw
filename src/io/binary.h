#ifndef __BINARY_H
#define __BINARY_H

#include "pico/binary_info.h"

#include "hardware/gpio.h"

#include "../config.h"
#include "../version.h"

/* This file compiles all pin definitions into the binary. */

/**
 * Declares all relavent information from pico-fbw into the binary.
 * Must be called once, anywhere in the program.
 * Location does not matter everything is compiled into the binary beforehand.
*/
void declare_binary() {
    /* Pin defs */

    // IMU communication pins (I2C)
    bi_decl(bi_2pins_with_func(IMU_SDA_PIN, IMU_SCL_PIN, GPIO_FUNC_I2C));

    // PWM input pins
    bi_decl(bi_4pins_with_func(INPUT_AIL_PIN, INPUT_ELEV_PIN, INPUT_RUD_PIN, INPUT_SW_PIN, GPIO_FUNC_PIO0));
    // bi_decl(bi_1pin_with_func(INPUT_THR_PIN, GPIO_FUNC_PIO1));

    // PWM output pins
    bi_decl(bi_4pins_with_func(SERVO_AIL_PIN, SERVO_ELEV_PIN, SERVO_RUD_PIN, ESC_THR_PIN, GPIO_FUNC_PWM));

    /* Program info */

    bi_decl(bi_program_description("A fly-by-wire system designed for RC airplanes, for the Rasperry Pi Pico microcontroller."));
    bi_decl(bi_program_version_string(PICO_FBW_VERSION));
    bi_decl(bi_program_url("https://github.com/MylesAndMore/pico-fbw"));
    #ifdef API_ENABLED
        bi_decl(bi_program_feature("API enabled"));
    #else
        bi_decl(bi_program_feature("API disabled"));
    #endif
    #ifdef GPS_ENABLED
        // GPS communication pins (UART)
        bi_decl(bi_2pins_with_func(GPS_TX_PIN, GPS_RX_PIN, GPIO_FUNC_UART));
        bi_decl(bi_program_feature("GPS enabled"));
    #else
        bi_decl(bi_program_feature("GPS disabled"));
    #endif
    #ifdef WIFLY_ENABLED
        bi_decl(bi_program_feature("Wi-Fly enabled"));
    #else
        bi_decl(bi_program_feature("Wi-Fly disabled"));
    #endif
}

#endif // __BINARY_H
