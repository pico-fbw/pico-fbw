#include <stdbool.h>
#include "pico/stdlib.h"

#include "modes.h"
#include "../io/flash.h"
#include "../io/pwm.h"
#include "../io/imu.h"
#include "../lib/pidtune.h"
#include "../config.h"

#include "tune.h"

inertialAngles angles;
float rollAngle;
float pitchAngle;
float rollIn;
float pitchIn;

void mode_tune() {
    // The first four bytes of our data array will signify if we have run a calibration before, a value of 0.3 floating point corresponds to true in this case so we add that to the array
    float tuning_data[CONFIG_SECTOR_SIZE];
    tuning_data[0] = 0.3f;
    // Tune both roll and pitch PID
    for (uint8_t i = 1; i < 3; i++) {
        // Set up tuning, input variables depend on which PID we are tuning but everything else is the same
        if (i == 1) {
            pidtune_init(&rollIn, &rollAngle);
        } else if (i == 2) {
            pidtune_init(&pitchIn, &pitchAngle);
        }
        // TODO: find good values for autotuning process
        pidtune_setOutputStep(1.0);
        pidtune_setNoiseBand(2.0);
        pidtune_setLookbackSec(0.1);
        pidtune_setKpDivisor(TUNING_KP);
        pidtune_setTiDivisor(TUNING_TI);
        pidtune_setTdDivisor(TUNING_TD);
        // If the function returns true then tuning has completed
        while (pidtune_runtime()) {
            // Refresh data
            angles = imu_getAngles();
            rollAngle = angles.roll;
            pitchAngle = angles.pitch;
            rollIn = pwm_readDeg(0) - 90;
            pitchIn = pwm_readDeg(1) - 90;
            // Check if there are any control inputs being made, if so, stop tuning and revert to direct mode
            if (rollIn > DEADBAND_VALUE || rollIn < -DEADBAND_VALUE || pitchIn > DEADBAND_VALUE || pitchIn < -DEADBAND_VALUE) {
                pidtune_cancel();
                mode(DIRECT);
                break;
            }
        }
        // Make sure the calibration didn't fail, we can do this by checking to ensure the constants aren't zero
        if ((pidtune_getKp() != 0.0f) && (pidtune_getTi() != 0.0f) && (pidtune_getTd() != 0.0f)) {
            // Flash the tuning data to the second/third (1 and 2) sectors of flash
            tuning_data[1] = pidtune_getKp();
            tuning_data[2] = pidtune_getTi();
            tuning_data[3] = pidtune_getTd();
            flash_write(i, tuning_data);
        }
    }
    // Exit to normal mode
    mode(NORMAL);
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
