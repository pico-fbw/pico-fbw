/**
 * This file contains all replaceable constants/other config values used throughout the project.
 * It's mostly here for convenience, to ease configuration in different scenarios, for example using different brand or layout servos.
*/

// @section config

// Enable this if you are using two seperate servos for aileron control. Disabled by default.
// #define DUAL_AIL;

// Enable this if you will be using a channel on your tx/rx to change control modes. Enabled by default.
#define MODE_SWITCH_ENABLE
#ifdef MODE_SWITCH_ENABLE
  static const int MODE_SWITCH_PIN = 14;
#endif


// @section servo

// The frequency to run your servos at (most are 50 and you shouldn't have to touch this).
static const int SERVO_HZ = 50;

#ifdef DUAL_AIL
  // Pin that the PWM wire on the AILERON LEFT servo is connected to.
  static const int SERVO_AIL_L_PIN = 12;
  // Pin that the PWM wire on the AILERON RIGHT servo is connected to.
  static const int SERVO_AIL_R_PIN = 13;
#else
  // Pin that the PWM wire on the AILERON servo is connected to.
  static const int SERVO_AIL_PIN = 13;
#endif

// Pin that the PWM wire on the ELEVATOR servo is connected to.
static const int SERVO_ELEV_PIN = 11;

// Pin that the PWM wire on the RUDDER servo is connected to.
static const int SERVO_RUD_PIN = 10;