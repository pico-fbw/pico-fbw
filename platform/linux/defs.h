#pragma once

#include <time.h>

typedef timer_t __callback_id_t;

// These pins assume a Raspberry Pi, but these are just defaults and can be overriden at runtime.

// Flight control I/O pins
#define PIN_INPUT_AIL 17
#define PIN_SERVO_AIL 18
#define PIN_INPUT_ELE 27
#define PIN_SERVO_ELE 22
#define PIN_INPUT_RUD 23
#define PIN_SERVO_RUD 24
#define PIN_INPUT_THR 25
#define PIN_ESC_THR 12
#define PIN_INPUT_SWITCH 16
#define PIN_SERVO_BAY 20

// Sensor I/O pins
// On Linux, these probably won't be very useful due to the /dev/i2c and /dev/serial interfaces.
#define PIN_AAHRS_SDA 0
#define PIN_AAHRS_SCL 1
#define PIN_GPS_TX 14
#define PIN_GPS_RX 15

// Platform details
#if defined(__x86_64__) || defined(_M_X64)
    #define ARCH "x86_64"
#elif defined(__i386__) || defined(_M_IX86)
    #define ARCH "x86"
#elif defined(__aarch64__)
    #define ARCH "ARM64"
#elif defined(__ARM_ARCH)
    #define ARCH "ARM"
#else
    #define ARCH "Unknown"
#endif
#define PLATFORM_NAME ("Linux-" ARCH)
// If you ever make any changes to the platform code, it's a nice courtesy to increment this.
#define PLATFORM_VERSION "1.0.0"
// Platform features
#define PLATFORM_SUPPORTS_ADC 0
#define PLATFORM_SUPPORTS_WIFI FBW_BUILD_WWW

#define __printflike(fmtarg, firstvararg) __attribute__((__format__(__printf__, fmtarg, firstvararg)))
