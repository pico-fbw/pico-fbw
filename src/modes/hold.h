#pragma once

#include <stdbool.h>

/**
 * Initializes hold mode.
 * @return true if initialization was successful, false if not.
 */
bool hold_init();

/**
 * Executes one cycle of hold mode.
 */
void hold_update();
