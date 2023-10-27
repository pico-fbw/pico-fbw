/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <math.h>
#include <stdio.h>
#include "pico/types.h"

#include "../../../../io/flash.h"

#include "get_pid.h"

uint api_get_pid(const char *cmd, const char *args) {
    printf("{\"roll\":{\"p\":");
    // In case you were wondering json is still dumb (look at get_config command for my reasoning)
    if (isfinite(flash_readFloat(FLOAT_SECTOR_PID, 1))) {
        printf("%f", flash_readFloat(FLOAT_SECTOR_PID, 1));
    } else {
        printf("null");
    }
    printf(",\"i\":");
    if (isfinite(flash_readFloat(FLOAT_SECTOR_PID, 2))) {
        printf("%f", flash_readFloat(FLOAT_SECTOR_PID, 2));
    } else {
        printf("null");
    }
    printf(",\"d\":");
    if (isfinite(flash_readFloat(FLOAT_SECTOR_PID, 3))) {
        printf("%f", flash_readFloat(FLOAT_SECTOR_PID, 3));
    } else {
        printf("null");
    }
    printf("},\"pitch\":{\"p\":");
    if (isfinite(flash_readFloat(FLOAT_SECTOR_PID, 4))) {
        printf("%f", flash_readFloat(FLOAT_SECTOR_PID, 4));
    } else {
        printf("null");
    }
    printf(",\"i\":");
    if (isfinite(flash_readFloat(FLOAT_SECTOR_PID, 5))) {
        printf("%f", flash_readFloat(FLOAT_SECTOR_PID, 5));
    } else {
        printf("null");
    }
    printf(",\"d\":");
    if (isfinite(flash_readFloat(FLOAT_SECTOR_PID, 6))) {
        printf("%f", flash_readFloat(FLOAT_SECTOR_PID, 6));
    } else {
        printf("null");
    }
    printf("}}\n");
    return -1;
}
