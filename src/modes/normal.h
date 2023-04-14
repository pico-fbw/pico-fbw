#ifndef normal_h
#define normal_h

/**
 * Executes one cycle of the normal mode.
*/
void mode_normal();

/**
 * Initializes the PID controllers and other variables required to operate the normal mode.
*/
void mode_normalInit();

/**
 * Partly resets the inner workings of normal mode (namely the setpoints).
*/
void mode_normalReset();

#endif // normal_h
