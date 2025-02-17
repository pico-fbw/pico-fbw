#pragma once

#include <time.h>

#include "platform/helpers.h"
#include "platform/types.h"

typedef timer_t __callback_id_t;

typedef struct I2CMapping {
    u32 sda, scl;
    const char *device;
} I2CMapping;

typedef struct UARTMapping {
    u32 tx, rx;
    const char *device;
} UARTMapping;

// These pins assume a Raspberry Pi, but these are just defaults and can be overriden at runtime.
// Pin numbers are defined as gpiochip line numbers.

// Flight control I/O pins
#define DEFAULT_PIN_INPUT_AIL 17
#define DEFAULT_PIN_SERVO_AIL 18
#define DEFAULT_PIN_INPUT_ELE 27
#define DEFAULT_PIN_SERVO_ELE 22
#define DEFAULT_PIN_INPUT_RUD 23
#define DEFAULT_PIN_SERVO_RUD 24
#define DEFAULT_PIN_INPUT_THR 25
#define DEFAULT_PIN_ESC_THR 12
#define DEFAULT_PIN_INPUT_SWITCH 16
#define DEFAULT_PIN_SERVO_BAY 20

// Sensor I/O pins
#define DEFAULT_PIN_AAHRS_SDA 2
#define DEFAULT_PIN_AAHRS_SCL 3
#define DEFAULT_PIN_GPS_TX 14
#define DEFAULT_PIN_GPS_RX 15

// I/O pin mappings
// Since Linux deals with its I2C and serial devices through /dev interfaces and not directly through pins,
// we need to map the pins to the devices they are connected to.
// These often change on a per-board basis.
static const I2CMapping I2C_MAP[] = {
    {.sda = 2, .scl = 3, .device = "/dev/i2c-1"},
};
#define MAX_I2C_DEVICES count_of(I2C_MAP)
static const UARTMapping UART_MAP[] = {
    {.tx = 14, .rx = 15, .device = "/dev/serial0"},
};
#define MAX_UARTS count_of(UART_MAP)

// GPIO settings
#define GPIOCHIP_DEVICE "gpiochip0"
#define MAX_GPIOD_REQUESTS 64 // Should be as large as the highest GPIO line being used

// Platform details
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
#define PLATFORM_NAME ("Linux-" ARCH)
// If you ever make any changes to the platform code, it's a nice courtesy to increment this.
#define PLATFORM_VERSION "1.0.0"
// Platform features
#define PLATFORM_SUPPORTS_ADC 0
#define PLATFORM_SUPPORTS_WIFI FBW_BUILD_WWW

#define __printflike(fmtarg, firstvararg) __attribute__((__format__(__printf__, fmtarg, firstvararg)))
