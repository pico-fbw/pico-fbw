/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include "pico/time.h"

#include "../io/esc.h"
#include "../io/flash.h"
#include "../io/gps.h"

#include "../lib/pid.h"

#include "throttle.h"

static PIDController athr_c;

void throttle_init() {
    // GPS is required for speed mode, as we need to know the aircraft's current speed
    throttle.supportedMode = gps.isSupported() ? THRMODE_SPEED : THRMODE_THRUST;
    if (throttle.supportedMode == THRMODE_SPEED) {
        athr_c = (PIDController){flash.pid[PID_THROTTLE_KP], flash.pid[PID_THROTTLE_KI], flash.pid[PID_THROTTLE_KD], flash.pid[PID_THROTTLE_TAU],
                                 flash.control[CONTROL_THROTTLE_DETENT_IDLE], flash.control[CONTROL_THROTTLE_DETENT_MAX],
                                 flash.pid[PID_THROTTLE_INTEGMIN], flash.pid[PID_THROTTLE_INTEGMAX]};
        pid_init(&athr_c);
    }
}

void throttle_update() {
    static float escTarget = 0.0f, prevEscTarget = 0.0f;
    static ThrottleState state = THRSTATE_NORMAL;
    static uint64_t stateChangeAt = 0;
    switch (throttle.mode) {
        case THRMODE_THRUST:
            escTarget = throttle.target;
            break;
        case THRMODE_SPEED:
            pid_update(&athr_c, (double)throttle.target, (double)gps.speed);
            escTarget = (float)athr_c.out;
            break;
    }
    // Validate against performance limits
    // Below idle is valid--in THRUST mode this can be used to simply stop the electric motor,
    // and the PID controller will never bring the output below idle in SPEED mode, so thrust being below IDLE isn't validated
    if (escTarget > flash.control[CONTROL_THROTTLE_DETENT_MCT]) {
        if (state == THRSTATE_NORMAL) {
            // We've just exceeded max continuous thrust, note the current time
            state = THRSTATE_MCT_EXCEEDED;
            stateChangeAt = time_us_64();
        }
        // MCT is still being exceeded (within this if block), what to do here depends on the specific state
        switch (state) {
            case THRSTATE_MCT_EXCEEDED:
                if ((time_us_64() - stateChangeAt) > (uint64_t)(flash.control[CONTROL_THROTTLE_MAX_TIME] * 1E6f)) {
                    // MCT has been exceeded for too long, lock
                    state = THRSTATE_MCT_LOCK;
                }
                break;
            case THRSTATE_MCT_LOCK:
            case THRSTATE_MCT_COOLDOWN:
                // Lock back to MCT if being exceeded (for both lock and cooldown states)
                escTarget = flash.control[CONTROL_THROTTLE_DETENT_MCT];
                break;
            default:
                break;
        }
        
    }
    if (state == THRSTATE_MCT_LOCK && escTarget <= flash.control[CONTROL_THROTTLE_DETENT_MCT]) {
        // Thrust has just been reduced back from exceeding MCT
        state = THRSTATE_MCT_COOLDOWN;
        stateChangeAt = time_us_64();
    }
    if ((time_us_64() - stateChangeAt) > (uint64_t)(flash.control[CONTROL_THROTTLE_COOLDOWN_TIME] * 1E6f)) {
        state = THRSTATE_NORMAL; // Cooldown over
    }
    // Target is now within limits
    // Apply filtering to smooth out any rapid throttle changes, and send final value to ESC
    escTarget = lerp(prevEscTarget, escTarget, flash.control[CONTROL_THROTTLE_SENSITIVITY]);
    prevEscTarget = escTarget;
    esc_set((uint)flash.pins[PINS_ESC_THROTTLE], (uint16_t)(escTarget + 0.5f));
}

Throttle throttle = {
    .mode = THRMODE_THRUST,
    .supportedMode = THRMODE_THRUST,
    .target = 0.0f,
    .init = throttle_init,
    .update = throttle_update
};
