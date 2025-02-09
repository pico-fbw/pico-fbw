#pragma once

#include "esp_timer.h"
#include "sdkconfig.h"

typedef esp_timer_handle_t __callback_id_t;

// Flight control I/O pins
#define DEFAULT_PIN_INPUT_AIL 15
#define DEFAULT_PIN_SERVO_AIL 4
#define DEFAULT_PIN_INPUT_ELE 16
#define DEFAULT_PIN_SERVO_ELE 17
#define DEFAULT_PIN_INPUT_RUD 5
#define DEFAULT_PIN_SERVO_RUD 18
#define DEFAULT_PIN_INPUT_THR 19
#define DEFAULT_PIN_ESC_THR 21
#define DEFAULT_PIN_INPUT_SWITCH 22
#define DEFAULT_PIN_SERVO_BAY 23

// Sensor I/O pins
#define DEFAULT_PIN_AAHRS_SDA 14
#define DEFAULT_PIN_AAHRS_SCL 27
#define DEFAULT_PIN_GPS_TX 26
#define DEFAULT_PIN_GPS_RX 25

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

#define LFS_THREADSAFE 1 // FreeRTOS is multithreaded so we need to enable thread safety in littlefs
