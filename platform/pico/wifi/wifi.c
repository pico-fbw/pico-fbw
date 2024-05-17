/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/wifi.h"

#if PLATFORM_SUPPORTS_WIFI

// clang-format off

#include <string.h>
#include "lwip/ip_addr.h"
#include "pico/cyw43_arch.h"

#include "dhcp.h"
#include "dns.h"
#include "http.h"

// clang-format on

// https://github.com/sysprogs/PicoHTTPServer
// or https://github.com/earlephilhower/arduino-pico

// TODO: pico rndis driver so pico (non-w) can still have the web interface?
// see https://github.com/OpenStickCommunity/GP2040-CE/blob/main/lib/rndis/rndis.c

DHCPServer dhcp;
DNSServer dns;
HTTPServer server;
ip_addr_t gateway, netmask;

bool wifi_setup(const char *ssid, const char *pass) {
    // Check credentials and set up the access point network
    if (!ssid || strlen(ssid) < WIFI_SSID_MIN_LEN || strlen(ssid) > WIFI_SSID_MAX_LEN)
        return false;
    if (pass && (strlen(pass) < WIFI_PASS_MIN_LEN || strlen(pass) > WIFI_PASS_MAX_LEN))
        return false;
    cyw43_arch_enable_ap_mode(ssid, pass, pass ? CYW43_AUTH_WPA2_AES_PSK : CYW43_AUTH_OPEN);
    // Gateway will use the IP 192.168.4.1, with a netmask of 255.255.255.0
    IP4_ADDR(ip_2_ip4(&gateway), 192, 168, 4, 1);
    IP4_ADDR(ip_2_ip4(&netmask), 255, 255, 255, 0);
    if (!dhcp_server_init(&dhcp, &gateway, &netmask))
        return false;
    if (!dns_server_init(&dns, &gateway))
        return false;
    return http_server_init(&server, &gateway);
}

void wifi_periodic() {
    return; // Nothing to do, all wifi tasks are handled in the background through interrupts
}

#endif // PLATFORM_SUPPORTS_WIFI
