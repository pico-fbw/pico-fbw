#pragma once

#include <stdbool.h>
#include "platform/types.h"

/**
 * Initializes normal mode.
 */
void normal_init();

/**
 * Executes one cycle of normal mode.
 */
void normal_update();

/**
 * Fully de-initializes normal mode.
 */
void normal_deinit();

/**
 * Manually adjusts the setpoints of normal mode.
 * @param roll the roll setpoint
 * @param pitch the pitch setpoint
 * @param yaw the yaw setpoint
 * @param throttle the throttle setpoint
 * @return true if the setpoints were adjusted, false if it was prevented by the user (already manually inputting).
 */
bool normal_set(f32 roll, f32 pitch, f32 yaw, f32 throttle);
