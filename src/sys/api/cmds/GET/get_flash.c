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
    for (FloatSector s = FLOAT_SECTOR_MIN; s <= FLOAT_SECTOR_MAX; s++) {
        printf("{\"values\":[");
        for (uint v = 0; v <= (FLOAT_SECTOR_SIZE - 1); v++) {
            float value = flash_readFloat(s, v);
            if (v != (FLOAT_SECTOR_SIZE - 1)) {
                isfinite(value) ? printf("%f,", value) : printf("null,");
            } else {
                isfinite(value) ? printf("%f]},", value) : printf("null]},");
            }
        }
    }
    for (StringSector s = STRING_SECTOR_MIN; s <= STRING_SECTOR_MAX; s++) {
        const char *value = flash_readString(s);
        if (s != STRING_SECTOR_MAX) {
            value ? printf("\"%s\",", value) : printf("\"\",");
        } else {
            value ? printf("\"%s\"]}\n", value) : printf("\"\"]}\n");
        }
    }
    return 200;
}
