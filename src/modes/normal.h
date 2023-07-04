#ifndef normal_h
#define normal_h

/**
 * Initializes normal mode.
*/
void mode_normalInit();

/**
 * Executes one cycle of the normal mode.
*/
void mode_normal();

/**
 * Partly resets the inner workings of normal mode (namely the setpoints).
 * This is just in case we go back to normal mode,
 * we don't want to keep the previous setpoints and have the system snap back to them.
*/
void mode_normalSoftReset();

/**
 * Fully resets/de-initializes normal mode.
 * This also frees up the second core that it makes use of.
*/
void mode_normalDeinit();

#endif // normal_h
