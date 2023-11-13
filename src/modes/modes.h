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
typedef void (*aircraft_changeto_t)(Mode);
typedef Mode (*aircraft_getmode_t)();
typedef void (*aircraft_setaahrssafe_t)(bool);
typedef void (*aircraft_setgpssafe_t)(bool);

typedef struct Aircraft {
    /**
     * Runs the code of the system's currently selected mode.
    */
    aircraft_update_t update;
    /**
     * Transitions the aircraft to a specified mode.
     * @param mode The mode to transition to.
    */
    aircraft_changeto_t changeTo;
    /**
     * @return The current mode of the system.
    */
    aircraft_getmode_t getMode;
    /**
     * @param state Declares whether or not the AAHRS data is safe to use.
    */
    aircraft_setaahrssafe_t setAAHRSSafe;
    /**
     * @param state Declares whether or not the GPS data is safe to use.
    */
    aircraft_setgpssafe_t setGPSSafe;
} Aircraft;

extern Aircraft aircraft;

#endif // __MODES_H
