#pragma once

// Note: for these definitions to take effect, you must add your platform to the #if chain in platform/defs.h.
// Another note: x is a used as placeholder for an actual value that you should define.

// See time.c for information on what this is and what you should define it as.
typedef x __callback_id_t;

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
#define PIN_LED x

// Platform details
// These are purely informational for the user, and can really be whatever you want.
#define PLATFORM_NAME "Example Platform"
#define PLATFORM_VERSION "1.0.0" // If you ever make any changes to the platform code, it's a nice courtesy to increment this.
// Platform features
// These are extra features that while aren't required to run pico-fbw, can be useful to have.
// If your platform supports them in hardware and you're willing to write extra code, pico-fbw will automatically use them.

// Here are all of the possible features:

// ADC
#define PLATFORM_SUPPORTS_ADC 0
#if PLATFORM_SUPPORTS_ADC
    // The ADC requires writing an extra file 'adc.c', see a platform such as 'pico' for an example.
    // Take a look at platform/adc.h to see the functions you'll need to implement.

    // ADC information
    #define ADC_NUM_CHANNELS x // Number of individual input channels your ADC supports
    #define PIN_ADC_0 x        // First pin your ADC supports
// Add more as needed...
static const u32 ADC_PINS[] = {PIN_ADC_0}; // Array of all ADC pins you've defined
#endif

// Display
#define PLATFORM_SUPPORTS_DISPLAY 0
#if PLATFORM_SUPPORTS_DISPLAY
                                           // Note that that this assumes an I2C display with a driver such as the SSD1306.
    // This feature actually doesn't require any extra code; all you need to do is define these details.
    // Display information
    #define DISPLAY_WIDTH x  // Width of the display in pixels (typically 128)
    #define DISPLAY_HEIGHT x // Height of the display in pixels (typically 32 or 64, pico-fbw currently only uses 32)
    #define DISPLAY_MAX_LINE_LEN                                                                                               \
        x                      // Maximum number of characters per line (typically 15 or 16, each character is 8x8 pixels)
                               // Display i2c bus details
    #define DISPLAY_FREQ_KHZ x // Frequency of I2C i2c bus in kHz (typically 100-400)
    #define DISPLAY_ADDR 0xxx  // I2C address of the display
    #define PIN_DISPLAY_SDA x  // SDA pin of the display
    #define PIN_DISPLAY_SCL x  // SCL pin of the display
#endif

// Wi-Fi
#define PLATFORM_SUPPORTS_WIFI 0
// Wi-Fi requires writing an extra file 'wifi.c' (and probably some more as needed),
// see platforms such as 'pico' and 'esp' for examples.
// Take a look at platform/wifi.h to see the functions you'll need to implement.

// If your platform requires any more miscellaneous definitions, put them here.
// Check the other platforms for examples of what you might need to define.
