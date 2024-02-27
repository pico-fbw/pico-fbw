#pragma once

#include "pico/config.h" // For platform-specific defines (e.g. RASPBERRYPI_PICO_W)

// TODO: add these to default config struct

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

// Platform features
#define PLATFORM_SUPPORTS_WIFI 0
#define PLATFORM_NAME "Raspberry Pi Pico"
#define PLATFORM_HAL_VERSION "1.0.0"

#ifdef RASPBERRYPI_PICO_W
    #define CYW43_GPIO_OFFSET 30
    #undef PIN_LED
    #define PIN_LED (CYW43_WL_GPIO_LED_PIN + CYW43_GPIO_OFFSET) // See gpio.c for why this is done
    
    #undef PLATFORM_SUPPORTS_WIFI
    #define PLATFORM_SUPPORTS_WIFI 1
    #undef PLATFORM_NAME
    #define PLATFORM_NAME "Raspberry Pi Pico W"
    #undef PLATFORM_HAL_VERSION
    #define PLATFORM_HAL_VERSION "1.0.0"
#endif

#define PIN_FBW 22
