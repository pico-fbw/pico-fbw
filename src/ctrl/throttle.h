#pragma once

#include "platform/types.h"

typedef enum ThrottleMode {
    THRMODE_THRUST, // Allows setting the thrust of the throttle directly (0-100%, within ESC limits)
    THRMODE_SPEED,  // Allows setting the target speed (in kts.), where the autothrottle will work to keep that speed
                    // (within ESC limits)
} ThrottleMode;

typedef struct Throttle {
    ThrottleMode mode;
    ThrottleMode supportedMode; // Highest ThrottleMode supported in the system's current configuration
    f32 target;                 // Target speed [kts] or thrust [0-100] (depending on mode)
    /**
     * Initializes the throttle system (checks for highest supported mode and initializes it).
     */
    void (*init)();
    /**
     * Updates the throttle output according to flight system data.
     * @note As with most computational functions, this should be called as often as possible for best results.
     */
    void (*update)();
} Throttle;

extern Throttle throttle;
