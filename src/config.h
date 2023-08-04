/* IMPORTANT NOTE: All pins signifiy GPIO pin values, not physical pins!! Do NOT mix these up!!! */

#ifndef __CONFIG_H
#define __CONFIG_H



/** @section control */

/* Values from the reciever are multiplied by this in normal mode.
Smaller values mean handling will be more sluggish like a larger plane, and larger values mean handling will be more agile like a typical RC plane.
This will be quite a small value--the setpoint is calculated many times per second! */
#define SETPOINT_SMOOTHING_VALUE 0.00075

/* Decides how much the aileron input is scaled up/down to become the rudder input during turns, does not apply during direct mode. */
#define RUDDER_TURNING_VALUE 1.5

/* If the degree reading from any of the inputs are below this value, the inputs will be disregarded, does not apply during direct mode. */
#define DEADBAND_VALUE 2



/** @section general */

/* Define the type of mode switch you are using, 3-pos is default and recommended. */
#define SWITCH_3_POS
// #define SWITCH_2_POS

/* Define if you want the system to calibrate each of your input PWM channels seperately.
If not defined, the system will only sample channel 0 and apply that value to all channels. */
#define CONFIGURE_INPUTS_SEPERATELY

/* The maximum value the system will accept as a calibration offset value for PWM input signals.
If any of the calibration (aileron, elevator, rudder, or switch) channels are larger than this value, the system will throw the error FBW-500 and fail to initialize.
Increase this value if you are experiening error FBW-500, however note you may be unprotected from bad calibration data. */
#define MAX_CALIBRATION_OFFSET 20

/* The frequency to run your servos at (almost all are 50 and you shouldn't have to touch this). */
#define SERVO_HZ 50



/** @section pins
 * Note that all PWM input pins must be mapped to a GPIO pin on a PWM_B channel.
 * At the time of writing this is simply odd number GPIO pins, but be sure to check a pinout just in case if you plan on modifying the pins.
 * Output pins do not have this limitation (they are controllable by all PWM channels which are present on almost all pins).
*/

#define INPUT_AIL_PIN 1 // Pin that the PWM signal wire from the reciever AILERON channel is connected to.
#define SERVO_AIL_PIN 2 // Pin that the PWM wire on the AILERON servo is connected to.

#define INPUT_ELEV_PIN 3 // Pin that the PWM signal wire from the reciever ELEVATOR channel is connected to.
#define SERVO_ELEV_PIN 4 // Pin that the PWM wire on the ELEVATOR servo is connected to.

#define INPUT_RUD_PIN 5 // Pin that the PWM signal wire from the reciever RUDDER channel is connected to.
#define SERVO_RUD_PIN 6 // Pin that the PWM wire on the RUDDER servo is connected to.

#define MODE_SWITCH_PIN 7 // Pin that the PWM signa wire from the reciever SWITCH channel is connected to.



/** @section limits 
 * Keep in mind that there are hard limits imposed that are not accessible within this configuration (72 degrees of bank, 35 degrees of pitch up, and 20 degrees of pitch down).
*/

/* The maximum roll angle that the system will attempt to stabilize; a constant input is required to keep a roll within this and ROLL_LIMIT_HOLD. */
#define ROLL_LIMIT 33
/* The maximum roll angle that the system will allow, nothing higher is allowed. */
#define ROLL_LIMIT_HOLD 67

/* The maximum pitch angle that the system will hold and stabilize, nothing higher is allowed. */
#define PITCH_UPPER_LIMIT 30
/* The minimum pitch angle that the system will hold and stabilize, nothing lower is allowed.
This value DOES need to be negative! */
#define PITCH_LOWER_LIMIT -15

/* Note that all control limits apply to both movement directions, specify how much the SERVOS are allowed to move, and are ignored in direct mode. */

/* The maximum degree value the system is allowed to move the ailerons to. */
#define AIL_LIMIT 25

/* The maximum degree value the system is allowed to move the elevators to. */
#define ELEV_LIMIT 15

/* The maximum rudder input the yaw damper/turn coordinator is allowed to make. */
#define RUD_LIMIT 20



/** @section sensors
 * For sensor types, uncomment whichever type you are using.
*/

/* IMU types */
// #define IMU_BNO055
#define IMU_MPU6050

/* Note that these pins must line up with the Pico's I2C0 interface, see a pinout if you're not sure! */
#define IMU_SDA_PIN 16
#define IMU_SCL_PIN 17

/* Uncomment to enable GPS functionality.
There are no module types, almost all GPS modules use the NMEA-0183 standard so that is what is supported here. */
#define GPS_ENABLED
#ifdef GPS_ENABLED

	/* Almost all GPS modules use either 4600 or 9600 baud rates with 9600 being more common. 
	Check the documentation of your GPS module and find its baud rate if you are experiencing issues. */
	#define GPS_BAUDRATE 9600

	/* Define the type of command used to communicate with the GPS. */
	#define GPS_COMMAND_TYPE_PMTK
	// Please let me know if there's a command type you would like supported! MTK appears to be the most common.

	/* These pins must line up with the UART1 interface.
	These labels refer to the GPS's pins; the GPS's TX pin is connected to GPS_TX_PIN on the Pico. */
	#define GPS_TX_PIN 21
	#define GPS_RX_PIN 20

#endif

/* More to come in the future...? Let me know if there's an IMU/GPS module you would like supported! */



/** @section Wi-Fly
 * Wi-Fly is enabled when the Pico W is detected as the target board.
*/
#ifdef WIFLY_ENABLED

	/* Define your country to optimize Wi-Fly (not required but recommended).
	See the list of available countries here: https://github.com/georgerobotics/cyw43-driver/blob/main/src/cyw43_country.h */
	#define WIFLY_NETWORK_COUNTRY CYW43_COUNTRY_WORLDWIDE

	/* Edit to change the name of the network Wi-Fly creates */
	#define WIFLY_NETWORK_NAME "pico-fbw"
	/* Uncomment to enable password protection on the network */
	// #define WIFLY_NETWORK_USE_PASSWORD
	#ifdef WIFLY_NETWORK_USE_PASSWORD
		/* Edit to change the password of the network */
		#define WIFLY_NETWORK_PASSWORD "password"
	#endif

#endif



/** @section tuning 
 * Changing these values are for experts ONLY!! The system's behavior can be radically altered through these values
 * which could cause crashes or even injuries, so PLEASE be careful with these values and test thoroughly!
 * It is suggested that you read up on what each of these values do in a PID control loop before attempting to alter them.
*/

// TODO: this is commented out for now because I can't fix autotune for now lol, uncomment later
// #define PID_AUTOTUNE // comment out if you want to manually set PID gains. Only do this if you really know what you're doing!!
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
	 * Defining this option implements relay bias (comment to disable).
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
	/* PID constants for the roll axis. */
	#define roll_kP 1.0
	#define roll_kI 0.0025
	#define roll_kD 0.001

	/* PID constants for the pitch axis. */
	#define pitch_kP 1.0
	#define pitch_kI 0.0025
	#define pitch_kD 0.001
#endif

/* Miscellaneous roll PID tuning values. */
#define roll_tau 0.001
#define roll_integMin -50.0
#define roll_integMax 50.0
#define roll_kT 0.01

/* Miscellaneous pitch PID tuning values. */
#define pitch_tau 0.001
#define pitch_integMin -50.0
#define pitch_integMax 50.0
#define pitch_kT 0.01

/* PID constants for the yaw axis. */
#define yaw_kP 1.0
#define yaw_kI 0.0025
#define yaw_kD 0.001
#define yaw_tau 0.001
#define yaw_integMin -50.0
#define yaw_integMax 50.0
#define yaw_kT 0.01


// TODO: autopilot PID tuning--both these PIDs do NOT input directly into the servos but instead command a bank/pitch angle so tune them with this in mind
/* PID constants for the autopilot's lateral guidance. */
#define latGuid_kP 0.005
#define latGuid_kI 0.008
#define latGuid_kD 0.002
#define latGuid_tau 0.001
#define latGuid_lim 33 // The maximum roll angle the autopilot can command
#define latGuid_integMin -50.0
#define latGuid_integMax 50.0
#define latGuid_kT 0.01

/* PID constants for the autopilot's vertical guidance. */
#define vertGuid_kP 0.05
#define vertGuid_kI 0.0025
#define vertGuid_kD 0.001
#define vertGuid_tau 0.001
#define vertGuid_loLim -15 // The minimum pitch angle the autopilot can command
#define vertGuid_upLim 25 // The maximum pitch angle the autopilot can command
#define vertGuid_integMin -50.0
#define vertGuid_integMax 50.0
#define vertGuid_kT 0.01



/** @section debug/api
 * Here you can enable/disable debugging features and the API.
 * Ensure you enable either USB or UART output in /CMakeLists.txt (one directory up, not this directory).
 * Once you've done that, you are welcome to enable/disable whatever debugging/API options you wish by commenting/uncommenting them here.
*/
#if defined(LIB_PICO_STDIO_USB) || defined(LIB_PICO_STDIO_UART)
	#define FBW_DEBUG // Internal, do not touch

	/* Uncomment to enable the API over serial. */
	// TODO: comment this in final
	#define API_ENABLED
	#ifdef API_ENABLED
		/* Uncomment to wait for a "PING" command on power-up before booting. */
		// #define API_WAIT_ON_BOOT
	#endif

	/* Time (in ms) to wait on bootup for the serial interface to initialize. */
	#define BOOTUP_WAIT_TIME_MS 800

	/* Enabled by default (misc logs + warning and error statements): */
	#define FBW_DEBUG_printf    printf
	/* Uncomment/replace as necessary to enable: */
	#define IMU_DEBUG_printf       // printf
	#define GPS_DEBUG_printf       printf
	#define WIFLY_DEBUG_printf     // printf
	#define WIFLY_DUMP_DATA     0  // 1
	#define TCP_DEBUG_printf       // printf
	#define TCP_DUMP_DATA       0  // 1
	#define DHCP_DEBUG_printf      // printf
	#define DNS_DEBUG_printf       // printf
	#define DNS_DUMP_DATA       0  // 1
	/* Some more in-depth wireless debug options are provided by LWIP and can be found in io/wifly/lwipopts.h if necessary. */

#else
	#define FBW_DEBUG_printf
	#define WIFLY_DEBUG_printf
	#define WIFLY_DUMP_DATA    0
	#define TCP_DEBUG_printf
	#define TCP_DUMP_DATA      0
	#define DHCP_DEBUG_printf
	#define DNS_DEBUG_printf
	#define DNS_DUMP_DATA      0
#endif



/* End of configuration. */

#endif // __CONFIG_H
