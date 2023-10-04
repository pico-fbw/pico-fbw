/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdio.h>
#include "pico/types.h"

#include "../../../../io/flash.h"

#include "set_pid.h"

uint api_set_pid(const char *cmd, const char *args) {
    float pid[FLOAT_SECTOR_SIZE] = {FLAG_PID};
    // Parse arguments for paremeters, verify number of arguments and that they were positive, and write to flash
    if ((sscanf(args, "%f %f %f %f %f %f", &pid[1], &pid[2], &pid[3], &pid[4], &pid[5], &pid[6]) >= 6) &&
    pid[1] > 0 && pid[2] > 0 && pid[3] > 0 && pid[4] > 0 && pid[5] > 0 && pid[6] > 0) {
        flash_writeFloat(FLOAT_SECTOR_PID, pid, true);
        return 200;
    } else {
        return 400;
    }
}
