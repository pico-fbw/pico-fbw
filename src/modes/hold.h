#ifndef __HOLD_H
#define __HOLD_H

typedef enum HoldStatus {
    HOLD_AWAITING_TURN,
    HOLD_TURN_BEGUN,
    HOLD_TURN_INPROGRESS,
    HOLD_TURN_ENDING,
    HOLD_TURN_STABILIZING,
    HOLD_TURN_UNSCHEDULED
} HoldStatus;

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

/**
 * Initializes tune mode.
 * @return true if initialization was successful, false if not.
*/
bool mode_holdInit();

/**
 * Executes one cycle of hold mode.
*/
void mode_hold();

#endif // __HOLD_H
