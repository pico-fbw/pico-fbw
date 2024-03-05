#pragma once

#include "esp_timer.h"
#include "sdkconfig.h"

typedef esp_timer_handle_t CallbackID;

// Flight control I/O pins
#define PIN_INPUT_AIL 15
#define PIN_SERVO_AIL 4
#define PIN_INPUT_ELE 16
#define PIN_SERVO_ELE 17
#define PIN_INPUT_RUD 5
#define PIN_SERVO_RUD 18
#define PIN_INPUT_THR 19
#define PIN_ESC_THR 21
#define PIN_INPUT_SWITCH 22
#define PIN_SERVO_BAY 23

// Sensor I/O pins
#define PIN_AAHRS_SDA 14
#define PIN_AAHRS_SCL 27
#define PIN_GPS_TX 26
#define PIN_GPS_RX 25

// Status LED
#define PIN_LED 2

// Platform details
#define PLATFORM_NAME ("Espressif Systems " CONFIG_IDF_TARGET " (" CONFIG_IDF_TARGET_ARCH ")")
#define PLATFORM_VERSION ("1.0.0 (ESP-IDF " CONFIG_IDF_INIT_VERSION ")")
// Platform features
#define PLATFORM_SUPPORTS_WIFI 1

// In ESP-IDF, FreeRTOS is required, and it expects the entrypoint to be called app_main
// This define will rename main() to app_main() in main.c (as this file is included in main.c before main() is defined)
#define main app_main

#define LFS_THREADSAFE 1 // FreeRTOS is multithreaded so we need to enable thread safety in littlefs
