/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include "platform/helpers.h"
#include "platform/time.h"

#include "io/esc.h"
#include "io/gps.h"
#include "io/receiver.h"
#include "lib/pid.h"
#include "sys/configuration.h"

#include "throttle.h"

typedef enum ThrottleState {
    THRSTATE_NORMAL,
    THRSTATE_MCT_EXCEEDED,
    THRSTATE_MCT_LOCK,
    THRSTATE_MCT_COOLDOWN
} ThrottleState;

static ThrottleMode currentMode = THRMODE_THRUST;
static ThrottleMode supportedMode = THRMODE_THRUST;
static f32 target = 0.0f;
static PIDController athr_c;
// throttle_update() state variables
static f32 escTarget = 0.0f;
static f32 prevEscTarget = 0.0f;
static ThrottleState state = THRSTATE_NORMAL;
static u64 stateChangeAt = 0;

/**
 * Calculates the ESC target based on the current throttle mode.
 * @return the calculated ESC target value
 */
static f32 calculate_esc_target() {
    switch (currentMode) {
        case THRMODE_THRUST:
            return target;
        case THRMODE_SPEED:
            pid_update(&athr_c, (f64)target, (f64)gps.speed);
            return (f32)athr_c.out;
        default:
            return target;
    }
}

/**
 * Handles MCT (Max Continuous Thrust) state transitions when MCT is exceeded.
 */
static void handle_mct_exceeded() {
    switch (state) {
        case THRSTATE_MCT_EXCEEDED: {
            u64 mctTime = (u64)config.control[CONTROL_THROTTLE_MAX_TIME];
            if ((time_s() - stateChangeAt) > mctTime && mctTime != 0) {
                // MCT has been exceeded for too long, lock
                state = THRSTATE_MCT_LOCK;
            }
            break;
        }
        case THRSTATE_MCT_LOCK:
        case THRSTATE_MCT_COOLDOWN:
            // Lock back to MCT if being exceeded (for both lock and cooldown states)
            escTarget = calibration.esc[ESC_DETENT_MCT];
            break;
        default:
            break;
    }
}

/**
 * Validates and limits the ESC target against performance limits.
 */
static void validate_performance_limits() {
    // Below idle is valid--in THRUST mode this can be used to simply stop the electric motor, and the PID controller
    // will never bring the output below idle in SPEED mode, so thrust being below IDLE isn't validated
    if (escTarget > calibration.esc[ESC_DETENT_MCT]) {
        if (state == THRSTATE_NORMAL) {
            // We've just exceeded max continuous thrust, note the current time
            state = THRSTATE_MCT_EXCEEDED;
            stateChangeAt = time_s();
        }
        // MCT is still being exceeded, handle based on current state
        handle_mct_exceeded();
    }
}

/**
 * Handles state transitions when thrust is reduced after exceeding MCT.
 */
static void handle_mct_reduction() {
    if (state == THRSTATE_MCT_LOCK && escTarget <= calibration.esc[ESC_DETENT_MCT]) {
        // Thrust has just been reduced back from exceeding MCT
        state = THRSTATE_MCT_COOLDOWN;
        stateChangeAt = time_s();
    }
}

/**
 * Handles cooldown period completion.
 */
static void handle_cooldown_complete() {
    if ((time_s() - stateChangeAt) > (u64)(config.control[CONTROL_THROTTLE_COOLDOWN_TIME])) {
        state = THRSTATE_NORMAL; // Cooldown over
    }
}

/**
 * Applies smoothing filter to throttle changes.
 * @return the filtered ESC target value
 */
static f32 apply_throttle_filtering() {
    f32 filtered = lerp(prevEscTarget, escTarget, config.control[CONTROL_THROTTLE_SENSITIVITY]);
    prevEscTarget = filtered;
    return filtered;
}

void throttle_init() {
    // GPS is required for speed mode, as we need to know the aircraft's current speed
    supportedMode = gps.is_supported() ? THRMODE_SPEED : THRMODE_THRUST;
    if (supportedMode == THRMODE_SPEED) {
        athr_c = (PIDController){
            .kp = calibration.pid[PID_THROTTLE_KP],
            .ki = calibration.pid[PID_THROTTLE_KI],
            .kd = calibration.pid[PID_THROTTLE_KD],
            .tau = calibration.pid[PID_TAU],
            .limMin = calibration.esc[ESC_DETENT_IDLE],
            .limMax = calibration.esc[ESC_DETENT_MAX],
        };
        pid_init(&athr_c);
    }
}

void throttle_update() {
    // Calculate base target from mode
    escTarget = calculate_esc_target();
    // Validate against performance limits
    validate_performance_limits();
    // Handle state transitions
    handle_mct_reduction();
    handle_cooldown_complete();
    // Apply filtering to smooth out any rapid throttle changes
    escTarget = apply_throttle_filtering();
    // Send final value to ESC
    esc_set((i16)config.pins[PINS_ESC_THROTTLE], (u16)(escTarget + 0.5f));
}

ThrottleMode throttle_get_mode() {
    return currentMode;
}

ThrottleMode throttle_get_supported_mode() {
    return supportedMode;
}

f32 throttle_get_target() {
    return target;
}

void throttle_set_mode(ThrottleMode mode) {
    currentMode = mode;
    if (mode == THRMODE_THRUST) {
        // Prime throttle with current value so that we don't get a big jump on first update
        escTarget = receiver_get((i16)config.pins[PINS_INPUT_THROTTLE], RECEIVER_MODE_PERCENT);
        prevEscTarget = escTarget;
    }
}

void throttle_set_target(f32 new_target) {
    target = new_target;
}
