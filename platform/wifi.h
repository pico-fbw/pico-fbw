#pragma once

#include "platform/defs.h"

typedef enum WifiEnabled {
    WIFI_DISABLED,
    WIFI_ENABLED_OPEN,
    WIFI_ENABLED_PASS,
} WifiEnabled;

#if PLATFORM_SUPPORTS_WIFI

    #include <stdbool.h>

/**
 * Sets up the Wi-Fi access point with the given SSID and password.
 * @param ssid the name of the access point network to create
 * @param pass the password for the access point network, or NULL for an open network
 * @return true if the access point was successfully created
 */
bool wifi_setup(const char *ssid, const char *pass);

/**
 * Runs periodic tasks relating to the Wi-Fi access point.
 * Should be called somewhat often to keep the access point running and responsive.
 */
void wifi_periodic();

#endif