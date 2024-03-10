#pragma once

// Flight control I/O pins
// These are all required; their numbers depend on your platform and where it allows certain GPIO functions such as PWM.
#define PIN_INPUT_AIL x
#define PIN_SERVO_AIL x
#define PIN_INPUT_ELE x
#define PIN_SERVO_ELE x
#define PIN_INPUT_RUD x
#define PIN_SERVO_RUD x
#define PIN_INPUT_THR x
#define PIN_ESC_THR x
#define PIN_INPUT_SWITCH x
#define PIN_SERVO_BAY x

// Sensor I/O pins
// These are also required and again depend on your platform.
#define PIN_AAHRS_SDA x
#define PIN_AAHRS_SCL x
#define PIN_GPS_TX x
#define PIN_GPS_RX x

// Status LED
// This is optional. Define if your platform has a built-in LED that can be used for status indication.
// Otherwise, comment out the following line and all LED-related code will be omitted from the final build.
#define PIN_LED 2

// Platform details
// These are purely informational for the user and can be whatever you want.
#define PLATFORM_NAME "Example Platform"
#define PLATFORM_VERSION "1.0.0"
// Platform features
// These will be used to enable or disable certain features in the final build.
// If your platform supports a feature, define it as 1, otherwise, define it as 0 (do not comment it out).
// Keep in mind that you'll need to write platform-specific code for each feature you enable for them to actually work.
#define PLATFORM_SUPPORTS_WIFI 1

// See time.c for information on what this is and what you should define it as.
typedef x __callback_id_t;

// If your platform requires any more miscellaneous definitions, put them here.
// Check the other platforms for examples of what you might need to define.
