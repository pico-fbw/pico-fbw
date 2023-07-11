#ifndef __HOLD_H
#define __HOLD_H

#ifdef WIFLY_ENABLED

// The amount of time (in seconds) that the aircraft will fly straight for in the holding pattern, before turning back around 180 degrees.
#define HOLD_TIME_PER_LEG_S 30

// The bank angle to turn at when making a 180 in the holding pattern--needs to be positive!
#define HOLD_TURN_BANK_ANGLE 25
// The bank angle to turn at the end of the turn when we are about to intercept the heading--also needs to be positive!
#define HOLD_TURN_SLOW_BANK_ANGLE 5

// The value (in degrees) within which the bank angle will begin to be decreased.
#define HOLD_HEADING_DECREASE_WITHIN 10

// The value (in degrees) within which the heading will be considered intercepted.
#define HOLD_HEADING_INTERCEPT_WITHIN 2

#define HOLD_AWAITING_TURN 0
#define HOLD_TURN_BEGUN 1
#define HOLD_TURN_INPROGRESS 2
#define HOLD_TURN_ENDING 3
#define HOLD_TURN_STABILIZING 4
#define HOLD_TURN_UNSCHEDULED 5

/**
 * Initializes tune mode.
*/
void mode_holdInit();

/**
 * Executes one cycle of hold mode.
*/
void mode_hold();

#endif // WIFLY_ENABLED

#endif // __HOLD_H
