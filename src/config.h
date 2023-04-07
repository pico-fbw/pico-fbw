/**
 * This file contains all replaceable constants/other config values used throughout the project.
 * It's mostly here for convenience, to ease configuration in different scenarios, for example using different brand or layout servos.
*/

/* IMPORTANT NOTE: All pins signifiy GPIO pin values, not physical pins!! */

#ifndef config_h
#define config_h

/** @section config */

// Enable this if you will be using a channel on your tx/rx to change control modes. Enabled by default.
#define MODE_SWITCH_ENABLE
#ifdef MODE_SWITCH_ENABLE
  	#define MODE_SWITCH_PIN 15
	// Define if the switch you are using is a two-position or three-position switch.
	#define SWITCH_3_POS
	// #define SWITCH_2_POS
#endif

// Define if you want the system to calibrate each of your input PWM channels seperately.
// If not defined, the system will only sample channel 0 (aileron) and apply those values to everything.
#define CONFIGURE_INPUTS_SEPERATELY

/** The value that decides how much values from the reciever are scaled down before being added to the setpoint value.
 * If that made absolutely no sense--basically, smaller values mean handling will be more like a larger plane, and larger values mean
 * handling will be more like a typical RC plane. This shouldn't be much higher than 1 (if at all).
*/
#define SETPOINT_SMOOTHING_VALUE 0.2


/** @section input */

// Pin that the PWM wire from the reciever AILERON channel is connected to.
#define INPUT_AIL_PIN 1

// Pin that the PWM wire from the reciever ELEVATOR channel is connected to.
#define INPUT_ELEV_PIN 3

// Pin that the PWM wire from the reciever RUDDER channel is connected to.
#define INPUT_RUD_PIN 7

/** @section servo */

// The frequency to run your servos at (most are 50 and you shouldn't have to touch this).
#define SERVO_HZ 50

// Pin that the PWM wire on the AILERON servo is connected to.
#define SERVO_AIL_PIN 13

// Pin that the PWM wire on the ELEVATOR servo is connected to.
#define SERVO_ELEV_PIN 12

// Pin that the PWM wire on the RUDDER servo is connected to.
#define SERVO_RUD_PIN 11


/** @section limits 
 * Keep in mind that there are hard limits imposed within normal mode.
 * If an angle of 72 degrees of bank, 35 degrees of pitch up, or 20 degrees of pitch down is detected from the IMU,
 * the system will revert back to direct mode. This is done for safety and computational/reliability reasons.
 * 
 * Most of the default values are the same values from an Airbus A320 aricraft--thanks to the FlyByWire team for the great info!
*/

// The maximum roll angle that the system will attempt to stabilize. A constant input is required above this.
#define ROLL_UPPER_LIMIT 33

// The maximum roll angle that the system will hold, nothing higher is allowed.
#define ROLL_UPPER_LIMIT_HOLD 67

// The minimum roll angle that the system will attempt to stabilize.
#define ROLL_LOWER_LIMIT -33

// The minimum roll angle that the system will hold, nothing higher is allowed.
#define ROLL_LOWER_LIMIT_HOLD -67

// The maximum pitch angle that the system will hold and stabilize, nothing higher is allowed.
#define PITCH_UPPER_LIMIT 30

// The minimum pitch angle that the system will hold and stabilize, nothing higher is allowed.
#define PITCH_LOWER_LIMIT -15


/** @section sensors */

// IMU Types--Define whichever IMU type you are using...
#define IMU_BNO055

#endif // config_h