#ifndef __MODES_H
#define __MODES_H

#include <stdbool.h>

#define MODE_MIN MODE_DIRECT
typedef enum Mode {
    MODE_DIRECT,
    MODE_NORMAL,
    MODE_AUTO,
    MODE_TUNE,
    MODE_HOLD
} Mode;
#define MODE_MAX MODE_HOLD

typedef void (*aircraft_update_t)();
typedef void (*aircraft_changeTo_t)(Mode);
typedef void (*aircraft_setAAHRSSafe_t)(bool);
typedef void (*aircraft_setGPSSafe_t)(bool);

typedef struct Aircraft {
    Mode mode;
    bool AAHRSSafe;
    bool GPSSafe;
    /**
     * Runs the code of the system's currently selected mode.
    */
    aircraft_update_t update;
    /**
     * Transitions the aircraft to a specified mode.
     * @param mode The mode to transition to.
    */
    aircraft_changeTo_t changeTo;
    /**
     * @param state Declares whether or not the AAHRS data is safe to use.
    */
    aircraft_setAAHRSSafe_t setAAHRSSafe;
    /**
     * @param state Declares whether or not the GPS data is safe to use.
    */
    aircraft_setGPSSafe_t setGPSSafe;
} Aircraft;

extern Aircraft aircraft;

#endif // __MODES_H
