#pragma once

#if defined(_WIN32)
    #include <windows.h>
#elif defined(__APPLE__)
    #include "time_apple.h"
#elif defined(__linux__)
    #include <time.h>
#else
    #warning "Unknown host platform, things may not work as expected."
#endif

#ifdef _WIN32
typedef HANDLE __callback_id_t;
#else
typedef timer_t __callback_id_t;
#endif

// All pins are set to -1 (unused) by default since host platforms have no I/O support.

#define DEFAULT_PIN_INPUT_AIL -1
#define DEFAULT_PIN_SERVO_AIL -1
#define DEFAULT_PIN_INPUT_ELE -1
#define DEFAULT_PIN_SERVO_ELE -1
#define DEFAULT_PIN_INPUT_RUD -1
#define DEFAULT_PIN_SERVO_RUD -1
#define DEFAULT_PIN_INPUT_THR -1
#define DEFAULT_PIN_ESC_THR -1
#define DEFAULT_PIN_INPUT_SWITCH -1
#define DEFAULT_PIN_SERVO_BAY -1

#define DEFAULT_PIN_AAHRS_SDA -1
#define DEFAULT_PIN_AAHRS_SCL -1
#define DEFAULT_PIN_GPS_TX -1
#define DEFAULT_PIN_GPS_RX -1

#if defined(_WIN32)
    #if SIMCONNECT
        #define PLATFORM "Windows+SimConnect"
    #else
        #define PLATFORM "Windows"
    #endif
#elif defined(__APPLE__)
    #define PLATFORM "macOS"
#elif defined(__linux__)
    #define PLATFORM "Linux+NoIO"
#else
    #define PLATFORM "Unknown"
#endif
#if defined(__x86_64__)
    #define ARCH "x86_64"
#elif defined(__i386__)
    #define ARCH "x86"
#elif defined(__aarch64__)
    #define ARCH "arm64"
#elif defined(__ARM_ARCH) || defined(__arm__)
    #define ARCH "arm"
#else
    #define ARCH "unknown"
#endif

// Platform details
#define PLATFORM_NAME (PLATFORM "-" ARCH)
#define PLATFORM_VERSION "1.0.0"
// Platform features
#define PLATFORM_SUPPORTS_ADC 0
#define PLATFORM_SUPPORTS_WIFI FBW_BUILD_WWW

// printf format checking
#ifdef __APPLE__
    #include <sys/cdefs.h>
#endif
#ifndef __printflike
    #if defined(__GNUC__) || defined(__clang__)
        #define __printflike(fmtarg, firstvararg) __attribute__((__format__(__printf__, fmtarg, firstvararg)))
    #else
        #define __printflike(fmtarg, firstvararg)
    #endif
#endif
