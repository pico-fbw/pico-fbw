/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>
#include <stdint.h>
#include "../lib/pidtune.h"

#include "../io/flash.h"
#include "../io/pwm.h"
#include "../io/imu.h"
#include "../io/led.h"

#include "modes.h"
#include "flight.h"

#include "../config.h"

#include "tune.h"

#ifdef PID_AUTOTUNE
    inertialAngles iAngles;
    float rollAngle;
    float pitchAngle;
    float rollIn;
    float pitchIn;

    void mode_tune() {
        // Start blinking LED to signify we are calibrating
        led_blink(100);
        // The first four bytes of our data array will signify if we have run a calibration before, a value of 0.3 floating point corresponds to true in this case so we add that to the array
        float tuning_data[CONFIG_SECTOR_SIZE] = {0.3f};
        // Tune both roll and pitch PID
        for (uint8_t i = 1; i <= 2; i++) {
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
                iAngles = imu_getAngles();
                rollAngle = iAngles.roll;
                pitchAngle = iAngles.pitch;
                rollIn = pwm_readDeg(0) - 90;
                pitchIn = pwm_readDeg(1) - 90;
                // Check if there are any control inputs being made, if so, stop tuning and revert to direct mode
                if (rollIn > DEADBAND_VALUE || rollIn < -DEADBAND_VALUE || pitchIn > DEADBAND_VALUE || pitchIn < -DEADBAND_VALUE) {
                    pidtune_cancel();
                    toMode(DIRECT);
                    return;
                }
                if (!flight_checkEnvelope(rollAngle, pitchAngle)) {
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
                led_blink(2000);
                toMode(DIRECT);
                return;
            }
        }
        // Stop blinking LED
        led_blink_stop();
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
