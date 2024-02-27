/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
*/

#include "sys/print.h"

#include "wifly/wifly.h"

#include "get_fplan.h"

i32 api_get_fplan(const char *cmd, const char *args) {
    if (wifly_fplanExists()) {
        printraw("%s\n", wifly_getFplanJson());
        return -1;
    } else {
        return 403;
    }
}
