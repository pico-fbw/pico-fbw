#pragma once

typedef enum ThrottleMode {
    THRMODE_THRUST, // Allows setting the thrust of the throttle directly (0-100%, within ESC limits)
    THRMODE_SPEED // Allows setting the target speed (in kts.), where the autothrottle will work to keep that speed (within ESC limits)
} ThrottleMode;

typedef void (*throttle_init_t)();
typedef void (*throttle_update_t)();

typedef struct Throttle {
    ThrottleMode mode;
    // Highest ThrottleMode supported in the system's current configuration (some extra sensors are required for SPEED mode).
    // If this is set to THRUST mode
    ThrottleMode supportedMode;
    float target; // Target speed [kts] or thrust [0-100] (depending on mode)
    /**
     * Initializes the throttle system (checks for highest supported mode and initializes it).
    */
    throttle_init_t init;
    /**
     * Updates the throttle output according to flight system data.
     * @note As with most computational functions, this should be called as often as possible for best results.
    */
    throttle_update_t update;
} Throttle;

extern Throttle throttle;
