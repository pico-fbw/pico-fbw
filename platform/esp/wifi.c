/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/wifi.h"

#if PLATFORM_SUPPORTS_WIFI

// TODO: implement wifi
// https://github.com/sysprogs/https://github.com/espressif/esp-idf/tree/master/examples/protocols/http_server/restful_server/ may be a good starting point

bool wifi_setup(const char *ssid, const char *pass) {
    // ...
    return true;
}

void wifi_periodic() {
    // ...
}

#endif // PLATFORM_SUPPORTS_WIFI
