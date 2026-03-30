#pragma once

// Note: for these definitions to take effect, you must add your platform to the #if chain in platform/defs.h.
// Another note: x is a used as placeholder for an actual value that you should define.

// See time.c for information on what this is and what you should define it as.
typedef x __callback_id_t;

// Flight control I/O pins
// These are all required; their numbers depend on your platform and where it allows certain GPIO functions such as PWM.
#define DEFAULT_PIN_INPUT_AIL x
#define DEFAULT_PIN_SERVO_AIL x
#define DEFAULT_PIN_INPUT_ELE x
#define DEFAULT_PIN_SERVO_ELE x
#define DEFAULT_PIN_INPUT_RUD x
#define DEFAULT_PIN_SERVO_RUD x
#define DEFAULT_PIN_INPUT_THR x
#define DEFAULT_PIN_ESC_THR x
#define DEFAULT_PIN_INPUT_SWITCH x
#define DEFAULT_PIN_SERVO_BAY x

// Sensor I/O pins
// These are also required and again depend on your platform.
#define DEFAULT_PIN_I2C_SDA x
#define DEFAULT_PIN_I2C_SCL x
#define DEFAULT_PIN_GPS_TX x
#define DEFAULT_PIN_GPS_RX x

// Status LED
// This is optional. Define if your platform has a built-in LED that can be used for status indication.
// Otherwise, comment out the following line and all LED-related code will be omitted from the final build.
#define PIN_LED x

// Platform details
// These are purely informational for the user, and can really be whatever you want.
#define PLATFORM_NAME "Example Platform"
// If you ever make any changes to the platform code, it's a nice courtesy to increment this.
#define PLATFORM_VERSION "1.0.0"
// Platform features
// These are extra features that while aren't required to run pico-fbw, can be useful to have.
// If your platform supports them in hardware and you're willing to write extra code,
// pico-fbw will automatically make use of them.

// Here are all of the possible features:

// Wi-Fi
#define PLATFORM_SUPPORTS_WIFI 0
// Wi-Fi requires writing an extra file 'wifi.c' (and probably some more as needed),
// see platforms such as 'pico' and 'esp' for examples.
// Take a look at platform/wifi.h to see the functions you'll need to implement.

// If your platform requires any more miscellaneous definitions, put them here.
// Check the other platforms for examples of what you might need to define.
