// This file contains all replaceable constants and other common config values used throughout the project.

/* IMPORTANT NOTE: All pins signifiy GPIO pin values, not physical pins!! Do NOT mix these up!!! */

#ifndef config_h
#define config_h

/** @section config */

// Enable this if you will be using a channel on your tx/rx to change control modes (why would you not?). Enabled by default.
#define MODE_SWITCH_ENABLE
#ifdef MODE_SWITCH_ENABLE
  	#define MODE_SWITCH_PIN 15
	// Define the type of switch you are using, a 3-pos is recommended.
	#define SWITCH_3_POS
	// #define SWITCH_2_POS
#endif

// Define if you want the system to calibrate each of your input PWM channels seperately.
// If not defined, the system will only sample channel 0 (aileron) and apply those values to everything.
#define CONFIGURE_INPUTS_SEPERATELY

/**
 * The maximum value the system will accept as a calibration offset value for PWM input signals.
 * If any of the calibration (aileron, elevator, rudder, or switch) channels are larger than this value, the system will throw an error and fail to initialize.
 * Increase this value if you are experiening error FBW-500, however note you may be unprotected from bad calibration data.
*/
#define MAX_CALIBRATION_OFFSET 20

/**
 * The value that decides how much values from the reciever are scaled down before being added to the setpoint value.
 * Smaller values mean handling will be more like a larger plane, and larger values mean handling will be more like a typical RC plane.
 * This should be quite a small value--the setpoint is calculated many times per second!
*/
#define SETPOINT_SMOOTHING_VALUE 0.00075

// The value that decides how much the aileron inputs are scaled up/down directly to become the rudder inputs.
// Does not apply during direct mode.
#define RUDDER_TURNING_VALUE 1.5

// If the degrees reading from any of the inputs are below this value, the inputs will be disregarded.
// Does not apply during direct mode.
#define DEADBAND_VALUE 2


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
 * the system will revert back to direct mode.
 * This is done for safety, PLEASE do not modify those values (outside of this file) for the safety of yourself and others!
 * 
 * Most of the default values are the same values from an Airbus A320 aricraft--thanks to the FlyByWire team for the great documentation!
*/

// The maximum roll angle that the system will attempt to stabilize. A constant input is required above this.
#define ROLL_LIMIT 33

// The maximum roll angle that the system will hold, nothing higher is allowed.
#define ROLL_LIMIT_HOLD 67

// The maximum pitch angle that the system will hold and stabilize, nothing higher is allowed.
#define PITCH_UPPER_LIMIT 30

// The minimum pitch angle that the system will hold and stabilize, nothing lower is allowed.
// This value DOES need to be negative!!
#define PITCH_LOWER_LIMIT -15

/* Note that all control limits apply to both directions, specify how much the SERVOS move, and are bypassed in direct mode. */

// The maximum degree value the system is allowed to move the ailerons to.
#define AIL_LIMIT 25

// The maximum degree value the system is allowed to move the elevators to.
#define ELEV_LIMIT 15

// The maximum rudder input the yaw damper/turn coordinator is allowed to make.
#define RUD_LIMIT 50


/** @section tuning 
 * Changing these values are for experts ONLY!! The system's behavior can be radically altered through these values
 * which could cause crashes or even injuries, so PLEASE be careful with these values and test thoroughly!
 * 
 * It is suggested that you read up on what each of these values do in a PID control loop before attempting to alter them if you wish.
*/

#define PID_AUTOTUNE // comment out if you want to manually set PID gains. Only do this if you really know what you're doing!!
#ifdef PID_AUTOTUNE
	/**
     * Some example PID auto tuning rules:
     * { {  44, 24,   0 } }  ZIEGLER_NICHOLS_PI
     * { {  34, 40, 160 } }  ZIEGLER_NICHOLS_PID
     * { {  64,  9,   0 } }  TYREUS_LUYBEN_PI
     * { {  44,  9, 126 } }  TYREUS_LUYBEN_PID
     * { {  66, 80,   0 } }  CIANCONE_MARLIN_PI
     * { {  66, 88, 162 } }  CIANCONE_MARLIN_PID
     * { {  28, 50, 133 } }  PESSEN_INTEGRAL_PID
     * { {  60, 40,  60 } }  SOME_OVERSHOOT_PID
     * { { 100, 40,  60 } }   NO_OVERSHOOT_PID
    */
   	// PID autotuning parameters.
	#define TUNING_KP 100
	#define TUNING_TI 40
	#define TUNING_TD 60

	// TODO: make sure these values actually work?

	/**
	 * Defining this option implements relay bias.
	 * This is useful to adjust the relay output values during the auto tuning to recover symmetric oscillations.
	 * This can compensate for load disturbance and equivalent signals arising from nonlinear or non-stationary processes.
	 * Any improvement in the tunings seems quite modest but sometimes unbalanced oscillations can be persuaded to converge where they might not otherwise have done so.
	*/ 
	#define AUTOTUNE_RELAY_BIAS

	/**
	 * Average amplitude of successive peaks must differ by no more than this proportion,
	 * relative to half the difference between maximum and minimum of last 2 cycles.
	*/ 
	#define AUTOTUNE_PEAK_AMPLITUDE_TOLERANCE 0.05

	/** 
	 * Ratio of up/down relay step duration should differ by no more than this tolerance.
	 * Biasing the relay con give more accurate estimates of the tuning parameters but setting the tolerance too low will prolong the autotune procedure unnecessarily.
	 * This parameter also sets the minimum bias in the relay as a proportion of its amplitude.
	*/ 
	#define AUTOTUNE_STEP_ASYMMETRY_TOLERANCE 0.50

	/**
	 * Auto tune terminates if waiting too long between peaks or relay steps.
	 * Set a larger value for processes with long delays or time constants.
	*/ 
	#define AUTOTUNE_MAX_WAIT_MINUTES 0.1
#else
	// PID tuning values for the roll axis PID controller.
	#define roll_kP 1.0
	#define roll_kI 0.0025
	#define roll_kD 0.001

	// PID tuning values for the pitch axis PID controller.
	#define pitch_kP 1.0
	#define pitch_kI 0.0025
	#define pitch_kD 0.001
#endif

// Miscellaneous roll PID tuning values.
#define roll_tau 0.001
#define roll_integMin -50.0
#define roll_integMax 50.0
#define roll_kT 0.01

// Miscellaneous pitch PID tuning values.
#define pitch_tau 0.001
#define pitch_integMin -50.0
#define pitch_integMax 50.0
#define pitch_kT 0.01

// PID tuning values for the yaw axis PID controller.
#define yaw_kP 1.0
#define yaw_kI 0.0025
#define yaw_kD 0.001
#define yaw_tau 0.001
#define yaw_integMin -50.0
#define yaw_integMax 50.0
#define yaw_kT 0.01


/** @section sensors */

// IMU types
// Define whichever IMU type you are using.
#define IMU_BNO055
// More to come in the future...?



/* End of configuration. */

#endif // config_h
