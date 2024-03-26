#pragma once

#include "platform/defs.h"

#define WIFI_SSID_MIN_LEN 2
#define WIFI_SSID_MAX_LEN 32
#define WIFI_PASS_MIN_LEN 8
#define WIFI_PASS_MAX_LEN 63

#define WIFI_ENABLED_MIN WIFI_DISABLED
typedef enum WifiEnabled {
    WIFI_DISABLED,
    WIFI_ENABLED_OPEN,
    WIFI_ENABLED_PASS,
} WifiEnabled;
#define WIFI_ENABLED_MAX WIFI_ENABLED_PASS

#if PLATFORM_SUPPORTS_WIFI

// clang-format off

#include <stdbool.h>

// clang-format on

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

#endif // PLATFORM_SUPPORTS_WIFI
