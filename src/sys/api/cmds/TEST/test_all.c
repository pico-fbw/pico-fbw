/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
*/

#include <stdio.h>
#include "pico/platform.h"
#include "pico/types.h"

#include "sys/api/cmds/TEST/test_aahrs.h"
#include "sys/api/cmds/TEST/test_gps.h"
#include "sys/api/cmds/TEST/test_pwm.h"
#include "sys/api/cmds/TEST/test_servo.h"

#include "sys/api/cmds/TEST/test_all.h"

int api_test_all(const char *cmd, const char *args) {
    uint status[4];
    uint passed = 0;
    status[0] = api_test_aahrs(cmd, args);
    status[1] = api_test_gps(cmd, args);
    status[2] = api_test_pwm(cmd, args);
    status[3] = api_test_servo(cmd, args);
    // Omit throttle test as servo uses the same IO, which is what we care about here
    for (uint i = 0; i < count_of(status); i++) {
        if (status[i] == 200) passed++;
    }
    printf("========== TEST RESULTS ==========");
    printf("\nAAHRS: %d", status[0]);
    if (status[0] == 200) printf(" (PASSED)");
    printf("\nGPS:   %d", status[1]);
    if (status[1] == 200) printf(" (PASSED, VERIFY)");
    printf("\nPWM:   %d", status[2]);
    if (status[2] == 200) printf(" (PASSED)");
    printf("\nSERVO: %d", status[3]);
    if (status[3] == 200) printf(" (PASSED, VERIFY)");
    printf("\nTOTAL: %d/%d", passed, count_of(status));
    if (passed == count_of(status)) printf(" PASS");
    printf("\n==================================\n");
    if (passed == count_of(status)) {
        return 200;
    } else return 500;
}
