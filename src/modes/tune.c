#include <stdbool.h>
#include "pico/stdlib.h"

#include "modes.h"
#include "../io/flash.h"
#include "../io/pwm.h"
#include "../lib/pidtune.h"
#include "../config.h"

#include "tune.h"

float rollIn;
float pitchIn;
// An array to store the tuning data later
float tuning_data[CONFIG_SECTOR_SIZE];

void mode_tuneInit() {
    // The first four bytes will signify if we have run a calibration before, a value of 0.3 floating point corresponds to true in this case so we add that to the array
    tuning_data[0] = 0.3f;
    // TODO: other autotune init things

}

void mode_tune() {
    // Check if there are any control inputs being made, if so, stop tuning revert to direct mode
    rollIn = pwm_readDeg(0) - 90;
    pitchIn = pwm_readDeg(1) - 90;
    if (rollIn > DEADBAND_VALUE || rollIn < -DEADBAND_VALUE || pitchIn > DEADBAND_VALUE || pitchIn < -DEADBAND_VALUE) {
        pidtune_cancel();
        mode(DIRECT);
    } else {
        pidtune_runtime();
    }
    // TODO: this if statement should activate when tuning is complete
    if (false) {
        // Flash the tuning data to the second sector of flash
        flash_write(1, tuning_data);
    }
}

bool mode_tuneCheckCalibration() {
    // Check to see if PID constants have been manually defined, if so, we return calibration as complete so as not to recalibrate
    #ifndef PID_AUTOTUNE
        // TODO: check if this is the first time checking calibration somehow, if so, write the manual constants along w the calibration flag
        
        float tuning_data[CONFIG_SECTOR_SIZE];
        tuning_data[0] = 0.3f;

    #endif   

    // Read the first value from the second sector of flash and compare it to what we expect for the calibration flag
        if (flash_read(1, 0) == 0.3f) {
            return true;
        } else {
            return false;
        }
}
