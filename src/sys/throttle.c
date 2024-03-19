/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/int.h"
#include "platform/time.h"

#include "io/esc.h"
#include "io/gps.h"

#include "lib/pid.h"

#include "sys/configuration.h"

#include "throttle.h"

typedef enum ThrottleState { THRSTATE_NORMAL, THRSTATE_MCT_EXCEEDED, THRSTATE_MCT_LOCK, THRSTATE_MCT_COOLDOWN } ThrottleState;

static PIDController athr_c;

void throttle_init() {
    // GPS is required for speed mode, as we need to know the aircraft's current speed
    throttle.supportedMode = gps.is_supported() ? THRMODE_SPEED : THRMODE_THRUST;
    if (throttle.supportedMode == THRMODE_SPEED) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
        athr_c = (PIDController){calibration.pid[PID_THROTTLE_KP],       calibration.pid[PID_THROTTLE_KI],
                                 calibration.pid[PID_THROTTLE_KD],       calibration.pid[PID_THROTTLE_TAU],
                                 calibration.esc[ESC_DETENT_IDLE],       calibration.esc[ESC_DETENT_MAX],
                                 calibration.pid[PID_THROTTLE_INTEGMIN], calibration.pid[PID_THROTTLE_INTEGMAX]};
        pid_init(&athr_c);
#pragma GCC diagnostic pop
    }
}

void throttle_update() {
    static float escTarget = 0.0f, prevEscTarget = 0.0f;
    static ThrottleState state = THRSTATE_NORMAL;
    static u64 stateChangeAt = 0;
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
    if (escTarget > calibration.esc[ESC_DETENT_MCT]) {
        if (state == THRSTATE_NORMAL) {
            // We've just exceeded max continuous thrust, note the current time
            state = THRSTATE_MCT_EXCEEDED;
            stateChangeAt = time_s();
        }
        // MCT is still being exceeded (within this if block), what to do here depends on the specific state
        switch (state) {
            case THRSTATE_MCT_EXCEEDED:
                if ((time_s() - stateChangeAt) > (u64)(config.control[CONTROL_THROTTLE_MAX_TIME])) {
                    // MCT has been exceeded for too long, lock
                    state = THRSTATE_MCT_LOCK;
                }
                break;
            case THRSTATE_MCT_LOCK:
            case THRSTATE_MCT_COOLDOWN:
                // Lock back to MCT if being exceeded (for both lock and cooldown states)
                escTarget = calibration.esc[ESC_DETENT_MCT];
                break;
            default:
                break;
        }
    }
    if (state == THRSTATE_MCT_LOCK && escTarget <= calibration.esc[ESC_DETENT_MCT]) {
        // Thrust has just been reduced back from exceeding MCT
        state = THRSTATE_MCT_COOLDOWN;
        stateChangeAt = time_s();
    }
    if ((time_s() - stateChangeAt) > (u64)(config.control[CONTROL_THROTTLE_COOLDOWN_TIME])) {
        state = THRSTATE_NORMAL; // Cooldown over
    }
    // Target is now within limits
    // Apply filtering to smooth out any rapid throttle changes, and send final value to ESC
    escTarget = lerp(prevEscTarget, escTarget, config.control[CONTROL_THROTTLE_SENSITIVITY]);
    prevEscTarget = escTarget;
    esc_set((u32)config.pins[PINS_ESC_THROTTLE], (u16)(escTarget + 0.5f));
}

Throttle throttle = {
    .mode = THRMODE_THRUST, .supportedMode = THRMODE_THRUST, .target = 0.0f, .init = throttle_init, .update = throttle_update};
