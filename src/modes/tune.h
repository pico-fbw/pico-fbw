#ifndef tune_h
#define tune_h

/**
 * Initializes parameters to begin auto tuning.
*/
void mode_tuneInit();

/**
 * Executes one cycle of the tune mode.
*/
void mode_tune();

/**
 * Checks if the tuning mode has been run before.
 * @return true if calibration has been run previously, false if not.
*/
bool mode_tuneCheckCalibration();

#endif // tune_h
