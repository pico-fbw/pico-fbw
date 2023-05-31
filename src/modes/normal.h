#ifndef normal_h
#define normal_h

/**
 * Executes one cycle of the normal mode.
*/
void mode_normal();

/**
 * Fully resets/de-initializes normal mode.
 * This also frees up the second core that it makes use of.
*/
void mode_normalDeinit();

/**
 * Partly resets the inner workings of normal mode (namely the setpoints).
*/
void mode_normalReset();

#endif // normal_h
