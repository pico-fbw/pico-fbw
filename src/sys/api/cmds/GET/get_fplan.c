/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
*/

#include <stdio.h>

#include "wifly/wifly.h"

#include "sys/api/cmds/GET/get_fplan.h"

int api_get_fplan(const char *cmd, const char *args) {
    if (wifly_fplanExists()) {
        printf("%s\n", wifly_getFplanJson());
        return -1;
    } else {
        return 403;
    }
}
