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
typedef Mode (*aircraft_mode_t)();
typedef void (*aircraft_setaahrssafe_t)(bool);
typedef bool (*aircraft_aahrssafe_t)();
typedef void (*aircraft_setgpssafe_t)(bool);
typedef bool (*aircraft_gpssafe_t)();

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
    aircraft_mode_t mode;
    /**
     * @param state Declares whether or not the AAHRS data is safe to use.
    */
    aircraft_setaahrssafe_t setAAHRSSafe;
    /**
     * @return Whether or not the AAHRS data is safe to use.
    */
    aircraft_aahrssafe_t AAHRSSafe;
    /**
     * @param state Declares whether or not the GPS data is safe to use.
    */
    aircraft_setgpssafe_t setGPSSafe;
    /**
     * @return Whether or not the GPS data is safe to use.
    */
    aircraft_gpssafe_t GPSSafe;
} Aircraft;

extern Aircraft aircraft;

#endif // __MODES_H
