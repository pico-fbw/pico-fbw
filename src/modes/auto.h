#ifndef __AUTO_H
#define __AUTO_H

/**
 * Initializes auto mode.
 * @return true if initialization was successful, false if not
*/
bool mode_autoInit();

#ifdef WIFLY_ENABLED

    #define INTERCEPT_RADIUS 0.5 // The radius at which to consider a waypoint "incercepted" in nm
    // TODO: do I need to change this for different speeds? idk if it will make too much of a difference, remember what aviation simmer said

    /**
     * Executes one cycle of auto mode.
    */
    void mode_auto();

#endif // WIFLY_ENABLED

/**
 * Fully de-initializes auto mode.
 * This also frees up the second core that it makes use of.
*/
void mode_autoDeinit();

#endif // __AUTO_H
