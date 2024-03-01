#pragma once

// TODO: define pins for esp32

// Status LED
#define PIN_LED 2

// Platform features
#define PLATFORM_SUPPORTS_WIFI 1
#define PLATFORM_NAME "Espressif Systems ESP32"
#define PLATFORM_HAL_VERSION "1.0.0"

// In ESP-IDF, FreeRTOS is basically required, and it expects the entrypoint to be called app_main
// This define will rename main() to app_main() in main.c (as this file is included in main.c before main() is defined)
#define main app_main
