/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>
#include <stdint.h>
#include "../lib/pidtune.h"

#include "../io/error.h"
#include "../io/flash.h"
#include "../io/pwm.h"
#include "../io/imu.h"
#include "../io/led.h"

#include "modes.h"
#include "flight.h"

#include "../config.h"

#include "tune.h"

#ifdef PID_AUTOTUNE
    static float rollInput;
    static float pitchInput;
    static double rollSet;
    static double pitchSet;

    void mode_tune() {
        // Start blinking LED to signify we are calibrating
        led_blink(100, 0);
        // Tune both roll and pitch PID
        for (uint8_t i = 1; i <= 2; i++) {
            // The first four bytes of our data array will signify if we have run a calibration before, a value of 0.3 floating point corresponds to true in this case so we add that to the array
            float tuning_data[CONFIG_SECTOR_SIZE] = {0.3f};
            // Set up tuning, input variables depend on which PID we are tuning but everything else is the same
            if (i == 1) {
                pidtune_init((double*)&aircraft.roll, &rollSet);
            } else if (i == 2) {
                pidtune_init((double*)&aircraft.pitch, &pitchSet);
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
                flight_update(rollSet, pitchSet, 0.0, true);

                // Check if there are any control inputs being made, if so, stop tuning and revert to direct mode
                rollInput = pwm_readDeg(0) - 90;
                pitchInput = pwm_readDeg(1) - 90;
                if (rollInput > DEADBAND_VALUE || rollInput < -DEADBAND_VALUE || pitchInput > DEADBAND_VALUE || pitchInput < -DEADBAND_VALUE) {
                    pidtune_cancel();
                    toMode(DIRECT);
                    return;
                }
            }
            // Make sure the calibration didn't fail, we can do this by checking to ensure the constants aren't zero
            if ((pidtune_getKp() != 0.0f) && (pidtune_getTi() != 0.0f) && (pidtune_getTd() != 0.0f)) {
                // Flash the tuning data to the second/third (1 and 2) sectors of flash
                tuning_data[1] = pidtune_getKp();
                tuning_data[2] = pidtune_getTi();
                tuning_data[3] = pidtune_getTd();
                flash_write(i, tuning_data);
            } else {
                // If calibration did fail, throw an error and revert to direct mode
                error_throw(ERROR_PID, ERROR_LEVEL_ERR, 2000, "PID tuning failed!");
                toMode(DIRECT);
                return;
            }
        }
        // Stop blinking LED
        led_stop();
        // Exit to normal mode
        toMode(NORMAL);
    }
#endif

bool mode_tuneCheckCalibration() {
    // Check to see if PID constants have not been manually defined
    #ifdef PID_AUTOTUNE
        // Read the first value from the second sector of flash and compare it to what we expect for the calibration flag
        if ((flash_read(FLASH_SECTOR_PID0, 0) == 0.3f) && (flash_read(FLASH_SECTOR_PID1, 0) == 0.3f)) {
            return true;
        } else {
            return false;
        }
    #else
        // If not, we return calibration as complete so as not to "calibrate" if manual constants are defined
        return true;
    #endif   
}
