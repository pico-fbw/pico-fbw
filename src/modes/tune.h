#ifndef __TUNE_H
#define __TUNE_H

/**
 * Executes one cycle of tune mode.
*/
void tune_update();

/**
 * Checks if the tuning mode has been run before.
 * @return true if calibration has been run previously, false if not.
*/
bool tune_isCalibrated();

#endif // __TUNE_H
