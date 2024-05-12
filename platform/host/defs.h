#pragma once

#if defined(_WIN32)
    #include <windows.h>
typedef HANDLE __callback_id_t;
#elif defined(__APPLE__)
    #include "time_apple.h"
typedef timer_t __callback_id_t;
#elif defined(__linux__)
    #include <time.h>
typedef timer_t __callback_id_t;
#else
    #warning "Unknown host platform, things may not work as expected."
#endif

// Not implemented
#define PIN_INPUT_AIL 0
#define PIN_SERVO_AIL 0
#define PIN_INPUT_ELE 0
#define PIN_SERVO_ELE 0
#define PIN_INPUT_RUD 0
#define PIN_SERVO_RUD 0
#define PIN_INPUT_THR 0
#define PIN_ESC_THR 0
#define PIN_INPUT_SWITCH 0
#define PIN_SERVO_BAY 0

#define PIN_AAHRS_SDA 0
#define PIN_AAHRS_SCL 0
#define PIN_GPS_TX 0
#define PIN_GPS_RX 0

#if defined(_WIN32)
    #define PLATFORM "Windows"
#elif defined(__APPLE__)
    #define PLATFORM "macOS"
#elif defined(__linux__)
    #define PLATFORM "Linux"
#else
    #define PLATFORM "Unknown"
#endif
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

// Platform details
#define PLATFORM_NAME (PLATFORM "-" ARCH)
#define PLATFORM_VERSION "1.0.0"
// Platform features
#define PLATFORM_SUPPORTS_ADC 0
#define PLATFORM_SUPPORTS_DISPLAY 0
#define PLATFORM_SUPPORTS_WIFI 0

#if defined(_WIN32)
    #define NO_COLOR_OUTPUT // Windows console does not support ANSI color codes
#endif

// For __printflike:
#ifndef __printflike
    #if defined(_WIN32)
        #include <sal.h>
        #if defined(_MSC_VER) && _MSC_VER > 1400
            #define __printflike(fmtarg, firstvararg) _Printf_format_string_ fmtarg
        #else
            #define __printflike(fmtarg, firstvararg)
        #endif
    #elif defined(__APPLE__)
        #include <sys/cdefs.h>
    #elif defined(__linux__)
        #if defined(__GNUC__) && __GNUC__ >= 3
            #define __printflike(fmtarg, firstvararg) __attribute__((__format__(__printf__, fmtarg, firstvararg)))
        #else
            #define __printflike(fmtarg, firstvararg)
        #endif
    #endif
#endif
