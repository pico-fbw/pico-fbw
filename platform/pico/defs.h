#pragma once

#include "pico/config.h" // For platform-specific defines (e.g. RASPBERRYPI_PICO_W)
#include "pico/time.h"

#include "platform/int.h"

typedef alarm_id_t __callback_id_t;

// Flight control I/O pins
#define PIN_INPUT_AIL 1
#define PIN_SERVO_AIL 2
#define PIN_INPUT_ELE 3
#define PIN_SERVO_ELE 4
#define PIN_INPUT_RUD 5
#define PIN_SERVO_RUD 6
#define PIN_INPUT_THR 7
#define PIN_ESC_THR 8
#define PIN_INPUT_SWITCH 9
#define PIN_SERVO_BAY 10

// Sensor I/O pins
#define PIN_AAHRS_SDA 16
#define PIN_AAHRS_SCL 17
#define PIN_GPS_TX 21
#define PIN_GPS_RX 20

// Status LED
#ifdef RASPBERRYPI_PICO
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
static const u32 ADC_PINS[] = {PIN_ADC_0, PIN_ADC_1, PIN_ADC_2, PIN_ADC_3};
#endif

// Display
#define PLATFORM_SUPPORTS_DISPLAY 1
#if PLATFORM_SUPPORTS_DISPLAY
    // Display information
    #define DISPLAY_WIDTH 128
    #define DISPLAY_HEIGHT 32
    #define DISPLAY_MAX_LINE_LEN 15 // 15 for margins; 16 can physically be fit but it looks bad
    // Display i2c bus details
    #define DISPLAY_FREQ_KHZ 400
    #define DISPLAY_ADDR 0x3C
    #define PIN_DISPLAY_SDA 18
    #define PIN_DISPLAY_SCL 19
#endif

// Wi-Fi
#define PLATFORM_SUPPORTS_WIFI 0

#ifdef RASPBERRYPI_PICO_W
    // To access GPIO on the CYW43 chip, add CYW43_GPIO_OFFSET to the GPIO number and use the typical GPIO functions
    #define CYW43_GPIO_OFFSET 30
    #undef PIN_LED
    #define PIN_LED (CYW43_WL_GPIO_LED_PIN + CYW43_GPIO_OFFSET) // See gpio.c for why this is done

    #undef PLATFORM_SUPPORTS_WIFI
    #define PLATFORM_SUPPORTS_WIFI 1
    #undef PLATFORM_NAME
    #define PLATFORM_NAME "Raspberry Pi Pico W"
#endif
