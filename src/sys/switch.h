#ifndef __SWITCH_H
#define __SWITCH_H

typedef enum SwitchType {
    SWITCH_TYPE_2_POS,
    SWITCH_TYPE_3_POS
} SwitchType;

typedef enum SwitchPosition {
    SWITCH_POSITION_LOW,
    SWITCH_POSITION_MID,
    SWITCH_POSITION_HIGH
} SwitchPosition;

/**
 * Updates the position of the mode switch.
 * @param pos The current position of the mode switch.
*/
void switch_update(SwitchPosition pos);

#endif // __SWITCH_H
