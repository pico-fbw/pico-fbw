#ifndef __AUTO_H
#define __AUTO_H

#ifdef GPS_ENABLED

    #define INTERCEPT_RADIUS 25 // The radius at which to consider a waypoint "incercepted" in meters
    // TODO: do I need to change this for different speeds? idk if it will make too much of a difference, remember what aviation simmer said

    /**
     * Initializes auto mode.
     * @return true if initialization was successful, false if not
    */
    bool mode_autoInit();

    /**
     * Executes one cycle of auto mode.
    */
    void mode_auto();

#endif // GPS_ENABLED

#endif // __AUTO_H
