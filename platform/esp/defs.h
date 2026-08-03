#pragma once

#include "esp_timer.h"
#include "sdkconfig.h"

typedef esp_timer_handle_t __callback_id_t;

// Flight control I/O pins
#define DEFAULT_PIN_INPUT_AIL 36
#define DEFAULT_PIN_SERVO_AIL 33
#define DEFAULT_PIN_INPUT_ELE 39
#define DEFAULT_PIN_SERVO_ELE 25
#define DEFAULT_PIN_INPUT_RUD 34
#define DEFAULT_PIN_SERVO_RUD 26
#define DEFAULT_PIN_INPUT_THR 35
#define DEFAULT_PIN_ESC_THR 26
#define DEFAULT_PIN_INPUT_SWITCH 32
#define DEFAULT_PIN_SERVO_BAY 14

// Sensor I/O pins
#define DEFAULT_PIN_I2C_SDA 16
#define DEFAULT_PIN_I2C_SCL 17
#define DEFAULT_PIN_SPI_CLK 18
#define DEFAULT_PIN_SPI_MOSI 23
#define DEFAULT_PIN_SPI_MISO 19
#define DEFAULT_PIN_SPI_CS 5
#define DEFAULT_PIN_GPS_TX 4
#define DEFAULT_PIN_GPS_RX 2

// Status LED
#define PIN_LED 2

// Platform details
#define PLATFORM_NAME ("Espressif Systems " CONFIG_IDF_TARGET " (" CONFIG_IDF_TARGET_ARCH ")")
#define PLATFORM_VERSION ("1.0.0 (ESP-IDF v" CONFIG_IDF_INIT_VERSION ")")
// Platform features
#define PLATFORM_SUPPORTS_WIFI FBW_BUILD_WWW

// In ESP-IDF, FreeRTOS is required, and it expects the entrypoint to be called app_main
// This define will rename main() to app_main() in main.c (as this file is included in main.c before main() is defined)
#define main app_main

// Recent ESP-IDF versions use picolibc instead of newlib, which defines __picolibc_format instead of __printflike
#ifndef __printflike
    #ifdef __picolibc_format
        #define __printflike(fmtarg, firstvararg) __picolibc_format(__printf__, fmtarg, firstvararg)
    #endif
#endif
