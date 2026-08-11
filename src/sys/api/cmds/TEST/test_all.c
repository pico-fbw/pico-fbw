/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include "platform/helpers.h"

#include "sys/print.h"

#include "test.h"

i32 api_test_all(const char *args) {
    u32 status[5];
    u32 passed = 0;
    status[0] = api_test_gps(args);
    status[1] = api_test_imu(args);
    status[2] = api_test_pwm(args);
    status[3] = api_test_servo(args);
    status[4] = api_test_throttle(args);
    for (u32 i = 0; i < count_of(status); i++) {
        if (status[i] == 200) {
            passed++;
        }
    }
    printraw("========== TEST RESULTS ==========");
    printraw("\nGPS:   %lu", status[0]);
    if (status[0] == 200) {
        printraw(" (PASSED, VERIFY)"); // "PASSED, VERIFY" results require more manual verification
    }
    printraw("\nIMU:   %lu", status[0]);
    if (status[1] == 200) {
        printraw(" (PASSED)");
    }
    printraw("\nPWM:   %lu", status[2]);
    if (status[2] == 200) {
        printraw(" (PASSED)");
    }
    printraw("\nSERVO: %lu", status[3]);
    if (status[3] == 200) {
        printraw(" (PASSED, VERIFY)");
    }
    printraw("\nTHROTTLE: %lu", status[4]);
    if (status[4] == 200) {
        printraw(" (PASSED, VERIFY)");
    }
    printraw("\nTOTAL: %lu/%i", passed, count_of(status));
    if (passed == count_of(status)) {
        printraw(" PASS");
    }
    printraw("\n==================================\n");
    if (passed == count_of(status)) {
        return 200;
    } else {
        return 500;
    }
}
