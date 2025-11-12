#pragma once

#include "platform/types.h"

typedef enum ThrottleMode {
    THRMODE_THRUST, // Allows setting the thrust of the throttle directly (0-100%, within ESC limits)
    THRMODE_SPEED,  // Allows setting the target speed (in kts.), where the autothrottle will work to keep that speed
                    // (within ESC limits)
} ThrottleMode;

/**
 * Initializes the throttle system (checks for highest supported mode and initializes it).
 */
void throttle_init();

/**
 * Updates the throttle output according to flight system data.
 * @note As with most computational functions, this should be called as often as possible for best results.
 */
void throttle_update();

/**
 * Gets the current throttle mode.
 * @return the current throttle mode
 */
ThrottleMode throttle_get_mode();

/**
 * Gets the highest supported throttle mode.
 * @return the highest supported throttle mode
 */
ThrottleMode throttle_get_supported_mode();

/**
 * Gets the current throttle target.
 * @return the target speed [kts] or thrust [0-100] (depending on mode)
 */
f32 throttle_get_target();

/**
 * Sets the throttle mode.
 * @param mode the throttle mode to set
 */
void throttle_set_mode(ThrottleMode mode);

/**
 * Sets the throttle target.
 * @param target the target speed [kts] or thrust [0-100] (depending on mode)
 */
void throttle_set_target(f32 target);
