/**
 * This file contains all replaceable constants/other config values used throughout the project.
 * It's mostly here for convenience, to ease configuration in different scenarios, for example using different brand or layout servos.
*/

#ifndef config_h
#define config_h

// @section config

// Enable this if you will be using a channel on your tx/rx to change control modes. Enabled by default.
#define MODE_SWITCH_ENABLE
#ifdef MODE_SWITCH_ENABLE
  	static const int MODE_SWITCH_PIN = 14;
#endif


/** @section input */

// Pin that the PWM wire from the reciever AILERON channel is connected to.
static const int INPUT_AIL_PIN = 1;

// Pin that the PWM wire from the reciever ELEVATOR channel is connected to.
static const int INPUT_ELEV_PIN = 3;

// Pin that the PWM wire from the reciever RUDDER channel is connected to.
static const int INPUT_RUD_PIN = 7;

/** @section servo */

// The frequency to run your servos at (most are 50 and you shouldn't have to touch this).
static const int SERVO_HZ = 50;

/* IMPORTANT NOTE: All of these pins signifiy GPIO pin values, not physical pins!! */

// Pin that the PWM wire on the AILERON servo is connected to.
static const int SERVO_AIL_PIN = 13;

// Pin that the PWM wire on the ELEVATOR servo is connected to.
static const int SERVO_ELEV_PIN = 12;

// Pin that the PWM wire on the RUDDER servo is connected to.
static const int SERVO_RUD_PIN = 11;


/** @section sensors */

// IMU Types--Define whichever IMU type you are using...
#define IMU_BNO055

#endif // config_h