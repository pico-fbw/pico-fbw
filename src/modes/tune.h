#pragma once

#include <stdbool.h>

/**
 * Executes one cycle of tune mode.
 */
void tune_update();

/**
 * Checks if the tuning mode has been run before.
 * @return true if tuning has been run previously.
 */
bool tune_is_tuned();
