/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <math.h>
#include <stdio.h>
#include "pico/types.h"

#include "../../../../io/flash.h"

#include "get_flash.h"

uint api_get_flash(const char *cmd, const char *args) {
    printf("{\"sectors\":[");
    float *fvalue = flash.calibration;
    for (uint s = 1; s <= NUM_FLOAT_SECTORS; s++) {
        printf("{\"values\":[");
        for (uint v = 0; v <= (FLOAT_SECTOR_SIZE - 1); v++) {
            if (v < (FLOAT_SECTOR_SIZE - 1)) {
                isfinite(*fvalue) ? printf("%f,", *fvalue) : printf("null,");
            } else {
                isfinite(*fvalue) ? printf("%f]},", *fvalue) : printf("null]},");
            }
            fvalue++;
        }
    }
    char *cvalue = flash.version;
    for (uint s = 1; s <= NUM_STRING_SECTORS; s++) {
        if (s < NUM_STRING_SECTORS) {
            cvalue ? printf("\"%s\",", cvalue) : printf("\"\",");
        } else {
            cvalue ? printf("\"%s\"]}\n", cvalue) : printf("\"\"]}\n");
        }
        cvalue += STRING_SECTOR_SIZE;
    }
    return -1;
}
