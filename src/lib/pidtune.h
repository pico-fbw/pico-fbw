#ifndef pidtune_h
#define pidtune_h

/**
 * Some example PID auto tuning rules:
 * { {  44, 24,   0 } },  // ZIEGLER_NICHOLS_PI
 * { {  34, 40, 160 } },  // ZIEGLER_NICHOLS_PID
 * { {  64,  9,   0 } },  // TYREUS_LUYBEN_PI
 * { {  44,  9, 126 } },  // TYREUS_LUYBEN_PID
 * { {  66, 80,   0 } },  // CIANCONE_MARLIN_PI
 * { {  66, 88, 162 } },  // CIANCONE_MARLIN_PID
 * { {  28, 50, 133 } },  // PESSEN_INTEGRAL_PID
 * { {  60, 40,  60 } },  // SOME_OVERSHOOT_PID
 * { { 100, 40,  60 } }   // NO_OVERSHOOT_PID
*/

// TODO: do I need to move any of these to the config file; are they that important?

/**
 * Defining this option implements relay bias.
 * This is useful to adjust the relay output values during the auto tuning to recover symmetric oscillations.
 * This can compensate for load disturbance and equivalent signals arising from nonlinear or non-stationary processes.
 * Any improvement in the tunings seems quite modest but sometimes unbalanced oscillations can be 
 * persuaded to converge where they might not otherwise have done so.
*/ 
#define AUTOTUNE_RELAY_BIAS

/**
 * Average amplitude of successive peaks must differ by no more than this proportion,
 * relative to half the difference between maximum and minimum of last 2 cycles.
*/ 
#define AUTOTUNE_PEAK_AMPLITUDE_TOLERANCE 0.05

/** 
 * Ratio of up/down relay step duration should differ by no more than this tolerance
 * Biasing the relay con give more accurate estimates of the tuning parameters but
 * setting the tolerance too low will prolong the autotune procedure unnecessarily.
 * This parameter also sets the minimum bias in the relay as a proportion of its amplitude.
*/ 
#define AUTOTUNE_STEP_ASYMMETRY_TOLERANCE 0.20

/**
 * Auto tune terminates if waiting too long between peaks or relay steps.
 * Set a larger value for processes with long delays or time constants.
*/ 
#define AUTOTUNE_MAX_WAIT_MINUTES 5



enum Peak {
    MINIMUM = -1,
    NOT_A_PEAK = 0,
    MAXIMUM = 1
};

enum AutoTunerState {
    AUTOTUNER_OFF = 0, 
    STEADY_STATE_AT_BASELINE = 1,
    STEADY_STATE_AFTER_STEP_UP = 2,
    RELAY_STEP_UP = 4,
    RELAY_STEP_DOWN = 8,
    CONVERGED = 16,
    FAILED = 128
}; 

// Tuning rule divisors
enum {
    KP_DIVISOR = 0,
    TI_DIVISOR = 1,
    TD_DIVISOR = 2
};

static const float CONST_PI          = 3.14159265358979323846;
static const float CONST_SQRT2_DIV_2 = 0.70710678118654752440;


/**
 * 
*/
void pidtune_init(float* input, float* output);

/**
 * 
*/
void pidtune_cancel();

/**
 * 
*/
bool pidtune_runtime();

/**
 * 
*/
float pidtune_processValueOffset(float avgStep1, float avgStep2);

/**
 * 
*/
void pidtune_setOutputStep(float step);

/**
 * 
*/
void pidtune_setControlType(unsigned char type);

/**
 * 
*/
void pidtune_setNoiseBand(float band);

/**
 * 
*/
void pidtune_setLookbackSec(int sec);

/**
 * 
*/
float pidtune_getKp();

/**
 * 
*/
float pidtune_getKi();

/**
 * 
*/
float pidtune_getKd();

#endif // pidtune_h
