#include <stdbool.h>
#include "pico/stdlib.h"

#include "../io/flash.h"

#include "tune.h"

void mode_tune() {
    // Create an array to store the tuning data
    float tuning_data[CONFIG_SECTOR_SIZE];
    // The first four bytes will signify if we have run a calibration before, a value of 0.3 floating point corresponds to true in this case so we add that to the array
    tuning_data[0] = 0.3f;
    // TODO: while loop to wait until PID tuning is complete and then--

    // Flash the tuning data to the second sector of flash
    flash_write(1, tuning_data);
}

bool mode_tuneCheckCalibration() {
    // Read the first value from the second sector of flash and compare it to what we expect for the calibration flag
    if (flash_read(1, 0) == 0.3f) {
        return true;
    } else {
        return false;
    }
}
