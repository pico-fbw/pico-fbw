/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/types.h"

#include "../../../../wifly/wifly.h"

#include "set_fplan.h"

uint api_set_fplan(const char *cmd, const char *args) {
    char *fplan = malloc(strlen(args) + 7);
    if (fplan != NULL) {
        // Format as an HTTP request for the parser
        sprintf(fplan, FPLAN_PARAM_CONCAT, args);
        bool status = wifly_parseFplan(fplan);
        free(fplan);
        if (status) {
            return 200;
        } else {
            return 500;
        }
    } else {
        return 500;
    }
}
