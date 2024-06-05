/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/wifi.h"

#if PLATFORM_SUPPORTS_WIFI

// clang-format off

#include "esp_netif.h"
#include "esp_wifi.h"

#include "dns.h"
#include "http.h"

#define WIFI_CHANNEL 1 // See https://en.wikipedia.org/wiki/List_of_WLAN_channels
#define WIFI_MAX_CONNECTIONS 4

// clang-format on

esp_netif_t *netif = NULL;
httpd_handle_t httpServer = NULL;
dns_server_handle_t dnsServer = NULL;

bool wifi_setup(const char *ssid, const char *pass) {
    if (!ssid || strlen(ssid) < WIFI_SSID_MIN_LEN || strlen(ssid) > WIFI_SSID_MAX_LEN)
        return false;
    if (pass && (strlen(pass) < WIFI_PASS_MIN_LEN || strlen(pass) > WIFI_PASS_MAX_LEN))
        return false;

    // Initialize underlying network stack
    // NVS is initialized in sys_boot_begin()
    if (esp_netif_init() != ESP_OK)
        return false;
    if (esp_event_loop_create_default() != ESP_OK)
        return false;
    netif = esp_netif_create_default_wifi_ap();

    // Initialize wifi access point
    wifi_init_config_t initConfig = WIFI_INIT_CONFIG_DEFAULT();
    if (esp_wifi_init(&initConfig) != ESP_OK)
        return false;
    // clang-format off
    wifi_config_t wifiConfig = {
        .ap = {
            // ssid and password are copied below, since thety are stored as arrays and not pointers
            .ssid_len = strlen(ssid),
            .channel = WIFI_CHANNEL,
            .max_connection = WIFI_MAX_CONNECTIONS,
            .authmode = pass ? WIFI_AUTH_WPA2_WPA3_PSK : WIFI_AUTH_OPEN,
            .pmf_cfg = {
                .required = true,
            },
        },
    };
    // clang-format on
    strncpy((char *)wifiConfig.ap.ssid, ssid, sizeof(wifiConfig.ap.ssid));
    if (pass)
        strncpy((char *)wifiConfig.ap.password, pass, sizeof(wifiConfig.ap.password));
    if (esp_wifi_set_mode(WIFI_MODE_AP) != ESP_OK)
        return false;
    if (esp_wifi_set_config(WIFI_IF_AP, &wifiConfig) != ESP_OK)
        return false;
    if (esp_wifi_start() != ESP_OK)
        return false;

    // Initialize the HTTP and DNS servers
    if (http_server_open(&httpServer) != ESP_OK)
        return false;
    dns_server_config_t dnsConfig = DNS_SERVER_CONFIG_SINGLE("*", "WIFI_AP_DEF");
    dnsServer = dns_server_start(&dnsConfig);
    return dnsServer != NULL;
}

void wifi_periodic() {
    return; // esp wifi handles events in background through tasks
}

bool wifi_disable() {
    if (dns_server_stop(dnsServer) != ESP_OK)
        return false;
    if (http_server_close(&httpServer) != ESP_OK)
        return false;
    if (esp_wifi_stop() != ESP_OK)
        return false;
    if (esp_wifi_set_mode(WIFI_MODE_NULL) != ESP_OK)
        return false;
    if (esp_wifi_deinit() != ESP_OK)
        return false;
    if (esp_event_loop_delete_default() != ESP_OK)
        return false;
    esp_netif_destroy_default_wifi(netif);
    esp_err_t deinit = esp_netif_deinit();
    return deinit == ESP_OK || deinit == ESP_ERR_NOT_SUPPORTED; // Deinit may not be supported, so this is the best we can do
}

#endif // PLATFORM_SUPPORTS_WIFI
