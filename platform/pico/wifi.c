/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/wifi.h"

#if PLATFORM_SUPPORTS_WIFI

    #include "pico/cyw43_arch.h"

// TODO: implement wifi
// https://github.com/sysprogs/PicoHTTPServer/ may be a good starting point

bool wifi_setup(const char *ssid, const char *pass) {
    if (!ssid)
        return false;
    cyw43_arch_enable_ap_mode(ssid, pass, pass ? CYW43_AUTH_WPA2_MIXED_PSK : CYW43_AUTH_OPEN);
    // ...
    return true;
}

void wifi_periodic() {
    // ...
}

#endif // PLATFORM_SUPPORTS_WIFI
