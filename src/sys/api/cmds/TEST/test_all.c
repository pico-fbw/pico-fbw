/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/helpers.h"

#include "sys/print.h"

#include "test_aahrs.h"
#include "test_gps.h"
#include "test_pwm.h"
#include "test_servo.h"
#include "test_throttle.h"

#include "test_all.h"

i32 api_test_all(const char *args) {
    u32 status[5];
    u32 passed = 0;
    status[0] = api_test_aahrs(args);
    status[1] = api_test_gps(args);
    status[2] = api_test_pwm(args);
    status[3] = api_test_servo(args);
    status[4] = api_test_throttle(args);
    for (u32 i = 0; i < count_of(status); i++) {
        if (status[i] == 200)
            passed++;
    }
    printraw("========== TEST RESULTS ==========");
    printraw("\nAAHRS: %lu", status[0]);
    if (status[0] == 200)
        printraw(" (PASSED)");
    printraw("\nGPS:   %lu", status[1]);
    if (status[1] == 200)
        printraw(" (PASSED, VERIFY)"); // "PASSED, VERIFY" results require more manual verification
    printraw("\nPWM:   %lu", status[2]);
    if (status[2] == 200)
        printraw(" (PASSED)");
    printraw("\nSERVO: %lu", status[3]);
    if (status[3] == 200)
        printraw(" (PASSED, VERIFY)");
    printraw("\nTHROTTLE: %lu", status[4]);
    if (status[4] == 200)
        printraw(" (PASSED, VERIFY)");
    printraw("\nTOTAL: %lu/%i", passed, count_of(status));
    if (passed == count_of(status))
        printraw(" PASS");
    printraw("\n==================================\n");
    if (passed == count_of(status)) {
        return 200;
    } else
        return 500;
}
