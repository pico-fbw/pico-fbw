/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdio.h>

#include "pico/binary_info.h"
#include "../lib/semver.h"

#include "hardware/gpio.h"

#include "../io/platform.h"

#include "../config.h"

#include "info.h"

/**
 * Declares all relavent information from pico-fbw into the binary.
 * Must be called once, anywhere in the program.
 * Location does not matter everything is compiled into the binary beforehand.
*/
void info_declare() {
    /* Pin defs */

    // I2C communication pins
    bi_decl(bi_2pins_with_func(IMU_SDA_PIN, IMU_SCL_PIN, GPIO_FUNC_I2C));
    bi_decl(bi_2pins_with_func(MARBE_D, MARBE_C, GPIO_FUNC_I2C));

    // PWM input pins
    #if defined(CONTROL_3AXIS)
        bi_decl(bi_4pins_with_func(INPUT_AIL_PIN, INPUT_ELEV_PIN, INPUT_RUD_PIN, INPUT_SW_PIN, GPIO_FUNC_PIO0));
        #ifdef ATHR_ENABLED
            bi_decl(bi_1pin_with_func(INPUT_THR_PIN, GPIO_FUNC_PIO1));
        #endif
    #elif defined(CONTROL_FLYINGWING)
        #ifndef ATHR_ENABLED
            bi_decl(bi_3pins_with_func(INPUT_AIL_PIN, INPUT_ELEV_PIN, INPUT_SW_PIN, GPIO_FUNC_PIO0));
        #else
            bi_decl(bi_4pins_with_func(INPUT_AIL_PIN, INPUT_ELEV_PIN, INPUT_SW_PIN, INPUT_THR_PIN, GPIO_FUNC_PIO0));
        #endif
    #endif

    // PWM output pins
    #if defined(CONTROL_3AXIS)
        #ifdef ATHR_ENABLED
            bi_decl(bi_4pins_with_func(SERVO_AIL_PIN, SERVO_ELEV_PIN, SERVO_RUD_PIN, ESC_THR_PIN, GPIO_FUNC_PWM));
        #else
            bi_decl(bi_3pins_with_func(SERVO_AIL_PIN, SERVO_ELEV_PIN, SERVO_RUD_PIN, GPIO_FUNC_PWM));
        #endif
    #elif defined(CONTROL_FLYINGWING)
        #ifndef ATHR_ENABLED
            bi_decl(bi_3pins_with_func(SERVO_ELEVON_L_PIN, SERVO_ELEVON_L_PIN, ESC_THR_PIN, GPIO_FUNC_PWM));
        #else
            bi_decl(bi_2pins_with_func(SERVO_ELEVON_L_PIN, SERVO_ELEVON_L_PIN, GPIO_FUNC_PWM));
        #endif
    #endif
    

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
        // UART communication pins
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

int info_checkVersion(const char *version) {
    // Ensure the input version isn't complete garbage
    if (version[0] == '\0') {
        return -2;
    }
    // Parse the version strings into the semantic versioning standard
    semver_t compare;
    if (semver_parse(version, &compare) < 0) {
        semver_free(&compare);
        FBW_DEBUG_printf("[version] unable to parse input version string!\n");
        return -3;
    }
    semver_t binary;
    if (semver_parse(PICO_FBW_VERSION, &binary) < 0) {
        semver_free(&compare);
        semver_free(&binary);
        FBW_DEBUG_printf("[version] unable to parse binary version string!\n");
        return -3;
    }
    // Compare the versions
    switch (semver_compare(binary, compare)) {
        case 0:
            return 0; // Equal
        case 1:
            // In most cases this will be a prerelease version but it could also be a user upgrading to a release from prerelease so check that case
            if (binary.prerelease[0] == '\0') {
                return -1; // Lower
            } else {
                FBW_DEBUG_printf("[version] thanks for testing %s :)\n", binary.prerelease);
                return 1; // Higher
            }
        case -1:
            return -1;
    }
}
