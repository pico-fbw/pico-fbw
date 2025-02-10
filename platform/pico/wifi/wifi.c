/**
 * Copyright (c) 2020 Peter Lawrence
 * influenced by lrndis https://github.com/fetisov/lrndis
 *
 * This file utilizes code under the MIT License. See "LICENSE" for details.
 */

// The virtual network interface is derived from the tinyusb example "net_lwip_webserver", thanks!

/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

#include "platform/wifi.h"

#if PLATFORM_SUPPORTS_WIFI

// clang-format off

#include <string.h>
#include "lwip/ip_addr.h"
#include "pico/config.h"
#ifdef RASPBERRYPI_PICO_W
    #include "pico/cyw43_arch.h"
#else
    #include "lwip/debug.h"
    #include "lwip/err.h"
    #include "lwip/etharp.h"
    #include "lwip/pbuf.h"
    #include "lwip/netif.h"
    #include "netif/ethernet.h"
    #include "pico/async_context_threadsafe_background.h"
    #include "pico/lwip_nosys.h"
    #include "pico/time.h"
    #include "pico/unique_id.h"
    #include "tusb.h"
#endif

#include "dhcp.h"
#include "dns.h"
#include "tcp.h"

#include "platform/types.h"

#define IP4(a, b, c, d) { PP_HTONL(LWIP_MAKEU32(a, b, c, d)) }
#define TCP_PORT 80

// clang-format on

static DHCPServer dhcp;
static DNSServer dns;
static TCPServer server;
static const ip_addr_t gateway = IP4(192, 168, 4, 1), netmask = IP4(255, 255, 255, 0);

    // If not on Pico W, we can use RNDIS/CDC-ECM to provide a virtual network interface when plugged in via USB
    #ifndef RASPBERRYPI_PICO_W

static async_context_threadsafe_background_t lwip_async_context;
static struct netif usb_net;
static struct pbuf *received_frame;
u8 tud_network_mac_address[6] = {[0 ... 5] = 0x00};

/**
 * Generate a MAC address for the device based on the unique board ID.
 * @param mac the buffer to store the generated MAC address in
 */
void generate_macaddr(u8 *mac) {
    pico_unique_board_id_t board_id;
    pico_get_unique_board_id(&board_id);
    memcpy(mac, &board_id.id[2], 6);
    mac[0] |= 0x02; // Set the LSbit to 1 to indicate a locally administered MAC address
    mac[0] &= 0xFE; // Clear the I/G bit to indicate a unicast MAC address
}

// lwIP callback. Will be called to transmit packets over the USB network interface.
static err_t usb_net_xmit_packet(struct netif *netif, struct pbuf *p) {
    while (true) {
        if (!tud_ready()) {
            return ERR_USE; // tinyusb not ready
        }
        if (tud_network_can_xmit(p->tot_len)) {
            tud_network_xmit(p, 0);
            return ERR_OK;
        }
        // Transfer execution to tinyusb to (hopefully) finish sending the packet
        tud_task();
    }
    (void)netif;
}

// lwIP callback. Will be called to initialize the USB network interface.
static err_t netif_init_cb(struct netif *netif) {
    LWIP_ASSERT("netif != NULL", (netif != NULL));
    netif->mtu = CFG_TUD_NET_MTU;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
    netif->state = NULL;
    netif->name[0] = 'E';
    netif->name[1] = 'X';
    netif->linkoutput = usb_net_xmit_packet;
    netif->output = etharp_output; // lwip ethernet output function
    return ERR_OK;
}

// tinyusb callback. Will be called when the network interface is initialized.
void tud_network_init_cb(void) {
    // If the network is re-initializing and we have a leftover packet
    if (received_frame) {
        pbuf_free(received_frame);
        received_frame = NULL;
    }
}

// tinyusb callback. Will be called when a packet is received over the USB network interface.
bool tud_network_recv_cb(const u8 *src, u16 size) {
    if (received_frame) {
        return false; // Haven't processed the previous packet yet, so we can't accept another
    }
    if (!size) {
        return true; // No data to copy
    }

    struct pbuf *p = pbuf_alloc(PBUF_RAW, size, PBUF_POOL);
    if (!p) {
        return false;
    }
    // pbuf_alloc() has already initialized struct; all we need to do is copy the data
    memcpy(p->payload, src, size);
    // usb_net_process_packets() will handle this pbuf later
    received_frame = p;
    return true;
}

// tinyusb callback. Will be called when a packet is ready to be transmitted over the USB network interface.
u16 tud_network_xmit_cb(u8 *dst, void *ref, u16 arg) {
    struct pbuf *p = (struct pbuf *)ref;
    return pbuf_copy_partial(p, dst, p->tot_len, 0);
    (void)arg;
}

/**
 * Initialize the USB network interface.
 * @return true if successfully initialized
 */
static bool usb_net_init() {
    // Initialize lwip stack
    async_context_threadsafe_background_config_t config = async_context_threadsafe_background_default_config();
    async_context_threadsafe_background_init(&lwip_async_context, &config);
    if (!lwip_nosys_init(&lwip_async_context.core)) {
        return false;
    }

    struct netif *netif = &usb_net;
    // Generate MAC address
    generate_macaddr(tud_network_mac_address);
    netif->hwaddr_len = sizeof(tud_network_mac_address);
    memcpy(netif->hwaddr, tud_network_mac_address, sizeof(tud_network_mac_address));
    LWIP_DEBUGF(NETIF_DEBUG,
                ("usb_net_init: generated MAC address %02X:%02X:%02X:%02X:%02X:%02X\n", netif->hwaddr[0],
                 netif->hwaddr[1], netif->hwaddr[2], netif->hwaddr[3], netif->hwaddr[4], netif->hwaddr[5]));
    // Add the usb network interface to lwip
    netif = netif_add(netif, &gateway, &netmask, &gateway, NULL, netif_init_cb, ip_input);
    if (!netif) {
        return false;
    }
    netif_set_default(netif);
    netif_set_up(netif);
    return true;
}

/**
 * Process any packets received by the USB network interface (tud_network_recv_cb()) through lwip.
 */
static void usb_net_process_packets() {
    if (!received_frame) {
        return;
    }
    // Process any packets received by the USB network interface (tud_network_recv_cb()) through lwip
    ethernet_input(received_frame, &usb_net);
    received_frame = NULL;
    tud_network_recv_renew();
}

    #endif // RASPBERRYPI_PICO_W

bool wifi_setup(const char *ssid, const char *pass) {
    #ifdef RASPBERRYPI_PICO_W
    // Check credentials and set up the access point network
    if (!ssid || strlen(ssid) < WIFI_SSID_MIN_LEN || strlen(ssid) > WIFI_SSID_MAX_LEN) {
        return false;
    }
    if (pass && (strlen(pass) < WIFI_PASS_MIN_LEN || strlen(pass) > WIFI_PASS_MAX_LEN)) {
        return false;
    }
    cyw43_arch_enable_ap_mode(ssid, pass, pass ? CYW43_AUTH_WPA3_WPA2_AES_PSK : CYW43_AUTH_OPEN);
    #else
    if (!usb_net_init()) {
        return false;
    }
    // If a device is currently connected via USB...
    if (tud_ready()) {
        // ...force re-enumeration to make the virtual network interface available
        tud_disconnect();
        sleep_ms(50);
        tud_connect();
        sleep_ms(1000);
    }
    (void)ssid;
    (void)pass;
    #endif
    if (!dhcp_server_init(&dhcp, &gateway, &netmask)) {
        return false;
    }
    if (!dns_server_init(&dns, &gateway)) {
        return false;
    }
    return tcp_server_open(&server, &gateway, TCP_PORT);
}

void wifi_periodic() {
    #ifdef RASPBERRYPI_PICO_W
    return; // Nothing to do, all wifi tasks are handled in the background through interrupts
    #else
    usb_net_process_packets();
    #endif
}

bool wifi_disable() {
    if (!tcp_server_close(&server)) {
        return false;
    }
    dns_server_deinit(&dns);
    dhcp_server_deinit(&dhcp);
    #ifdef RASPBERRYPI_PICO_W
    cyw43_arch_disable_ap_mode();
    // Don't run cyw43_arch_deinit(), as we still need the driver for LED, etc.
    #else
    netif_remove(&usb_net);
    lwip_nosys_deinit(&lwip_async_context.core);
    #endif
    return true;
}

#endif // PLATFORM_SUPPORTS_WIFI
