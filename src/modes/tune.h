#pragma once

#include <stdbool.h>

/**
 * Initializes tune mode.
 */
void tune_init();

/**
 * Executes one cycle of tune mode.
 */
void tune_update();

/**
 * Fully de-initializes tune mode.
 */
void tune_deinit();

/**
 * Checks if the tuning mode has been run before.
 * @return true if tuning has been run previously.
 */
bool tune_is_tuned();
