#pragma once

#define SWITCH_TYPE_MIN SWITCH_TYPE_2_POS
typedef enum SwitchType {
    SWITCH_TYPE_2_POS,
    SWITCH_TYPE_3_POS,
} SwitchType;
#define SWITCH_TYPE_MAX SWITCH_TYPE_3_POS

/**
 * Updates the mode switch's position and changes the aircraft's mode accordingly.
 */
void switch_update();
