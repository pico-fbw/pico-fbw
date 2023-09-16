/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>

#include "tune.h"

// TODO: autotune was too messy and iffy for now, going to come back to it later :)

#ifdef PID_AUTOTUNE
    void mode_tune() {
        
    }
#endif

bool mode_tuneisCalibrated() {
    // Check to see if PID constants have not been manually defined
    #ifdef PID_AUTOTUNE
        // Read the first value from the second sector of flash and compare it to what we expect for the calibration flag
        if ((flash_readFloat(FLASH_SECTOR_PID0, 0) == 0.3f)) {
            // TODO: maybe also ensure the values make sense?
            return true;
        } else {
            return false;
        }
    #else
        // If not, we return calibration as complete so as not to "calibrate" if manual constants are defined
        return true;
    #endif   
}
