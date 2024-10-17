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
#include "tcp.h"

#define TCP_PORT 80

// clang-format on

// TODO: pico rndis driver so pico (non-w) can still have the web interface?
// see https://github.com/OpenStickCommunity/GP2040-CE/blob/main/lib/rndis/rndis.c
// https://github.com/sidd-kishan/PicoPiFi

DHCPServer dhcp;
DNSServer dns;
TCPServer server;
ip_addr_t gateway, netmask;

bool wifi_setup(const char *ssid, const char *pass) {
    // Check credentials and set up the access point network
    if (!ssid || strlen(ssid) < WIFI_SSID_MIN_LEN || strlen(ssid) > WIFI_SSID_MAX_LEN)
        return false;
    if (pass && (strlen(pass) < WIFI_PASS_MIN_LEN || strlen(pass) > WIFI_PASS_MAX_LEN))
        return false;
    cyw43_arch_enable_ap_mode(ssid, pass, pass ? CYW43_AUTH_WPA3_WPA2_AES_PSK : CYW43_AUTH_OPEN);
    // Gateway will use the IP 192.168.4.1, with a netmask of 255.255.255.0
    IP4_ADDR(ip_2_ip4(&gateway), 192, 168, 4, 1);
    IP4_ADDR(ip_2_ip4(&netmask), 255, 255, 255, 0);
    if (!dhcp_server_init(&dhcp, &gateway, &netmask))
        return false;
    if (!dns_server_init(&dns, &gateway))
        return false;
    return tcp_server_open(&server, &gateway, TCP_PORT);
}

void wifi_periodic() {
    return; // Nothing to do, all wifi tasks are handled in the background through interrupts
}

bool wifi_disable() {
    if (!tcp_server_close(&server))
        return false;
    dns_server_deinit(&dns);
    dhcp_server_deinit(&dhcp);
    cyw43_arch_disable_ap_mode();
    // Don't run cyw43_arch_deinit(), as we still need the driver for things like the LED
    return true;
}

#endif // PLATFORM_SUPPORTS_WIFI
