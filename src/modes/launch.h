#pragma once

#include <stdbool.h>
#include "modes/aircraft.h"

/**
 * Initializes launch mode.
 * @param return_to the Mode to return to after the launch is complete
 * @return true if initialization was successful
 */
bool launch_init(Mode return_to);

/**
 * Executes one cycle of launch mode.
 */
void launch_update();
