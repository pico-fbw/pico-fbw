#pragma once

#include "pico/binary_info.h"
#include "pico/config.h" // For platform-specific defines (e.g. RASPBERRYPI_PICO_W)
#include "pico/time.h"

#include "platform/types.h"

typedef alarm_id_t __callback_id_t;

// Flight control I/O pins
#define DEFAULT_PIN_INPUT_AIL 1
#define DEFAULT_PIN_SERVO_AIL 2
#define DEFAULT_PIN_INPUT_ELE 3
#define DEFAULT_PIN_SERVO_ELE 4
#define DEFAULT_PIN_INPUT_RUD 5
#define DEFAULT_PIN_SERVO_RUD 6
#define DEFAULT_PIN_INPUT_THR 7
#define DEFAULT_PIN_ESC_THR 8
#define DEFAULT_PIN_INPUT_SWITCH 9
#define DEFAULT_PIN_SERVO_BAY 10

// Sensor I/O pins
#define DEFAULT_PIN_I2C_SDA 16
#define DEFAULT_PIN_I2C_SCL 17
#define DEFAULT_PIN_GPS_TX 21
#define DEFAULT_PIN_GPS_RX 20

// Status LED
#ifdef PICO_DEFAULT_LED_PIN
    #define PIN_LED PICO_DEFAULT_LED_PIN
#endif

// Platform details
#define PLATFORM_NAME "Raspberry Pi Pico"
#define PLATFORM_VERSION ("1.0.0 (pico-sdk v" PICO_SDK_VERSION_STRING ")")
// Platform features
// ADC
#define PLATFORM_SUPPORTS_ADC 1
#if PLATFORM_SUPPORTS_ADC
    // ADC information
    #define ADC_NUM_CHANNELS 4
    #define PIN_ADC_0 26
    #define PIN_ADC_1 27
    #define PIN_ADC_2 28
    #define PIN_ADC_3 29
static const i16 ADC_PINS[] = {PIN_ADC_0, PIN_ADC_1, PIN_ADC_2, PIN_ADC_3};
#endif

// Wi-Fi
// Technically, only the Pico W supports Wi-Fi, but other platforms emulate it through USB
#define PLATFORM_SUPPORTS_WIFI FBW_BUILD_WWW

#ifdef RASPBERRYPI_PICO2
    #undef PLATFORM_NAME
    #define PLATFORM_NAME "Raspberry Pi Pico 2"
#endif

#ifdef RASPBERRYPI_PICO_W
    // To access GPIO on the CYW43 chip, add CYW43_GPIO_OFFSET to the GPIO number and use the typical GPIO functions
    #define CYW43_GPIO_OFFSET 30
    #undef PIN_LED
    #define PIN_LED (CYW43_WL_GPIO_LED_PIN + CYW43_GPIO_OFFSET) // See gpio.c for why this is done

    #undef PLATFORM_NAME
    #define PLATFORM_NAME "Raspberry Pi Pico W"
#endif

bi_decl(bi_program_name("pico-fbw"));
bi_decl(bi_program_description(
    "A cost-effective, intuitive, and reliable remote control autopilot solution built for the future."));
bi_decl(bi_program_url("https://pico-fbw.org"));
bi_decl(bi_program_version_string(PICO_FBW_VERSION));
bi_decl(bi_program_build_date_string(__DATE__ " " __TIME__));
bi_decl(bi_program_build_attribute(PLATFORM_NAME));
bi_decl(bi_program_build_attribute(PLATFORM_VERSION));
