/**
 * This file utilizes code under the MIT License. See "LICENSE" for details.
 */

/**
 * This file is part of the MicroPython project, http://micropython.org/
 * https://github.com/micropython/micropython/blob/master/shared/netutils/dhcpserver.c
 *
 * For DHCP specs see:
 * https://www.ietf.org/rfc/rfc2131.txt
 * https://tools.ietf.org/html/rfc2132 -- DHCP Options and BOOTP Vendor Extensions
 */

/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "dhcp.h"

#if PLATFORM_SUPPORTS_WIFI

// clang-format off

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "lwip/debug.h"

#include "platform/time.h"

#define DHCPDISCOVER    (1)
#define DHCPOFFER       (2)
#define DHCPREQUEST     (3)
#define DHCPDECLINE     (4)
#define DHCPACK         (5)
#define DHCPNACK        (6)
#define DHCPRELEASE     (7)
#define DHCPINFORM      (8)

#define DHCP_OPT_PAD                (0)
#define DHCP_OPT_SUBNET_MASK        (1)
#define DHCP_OPT_ROUTER             (3)
#define DHCP_OPT_DNS                (6)
#define DHCP_OPT_HOST_NAME          (12)
#define DHCP_OPT_REQUESTED_IP       (50)
#define DHCP_OPT_IP_LEASE_TIME      (51)
#define DHCP_OPT_MSG_TYPE           (53)
#define DHCP_OPT_SERVER_ID          (54)
#define DHCP_OPT_PARAM_REQUEST_LIST (55)
#define DHCP_OPT_MAX_MSG_SIZE       (57)
#define DHCP_OPT_VENDOR_CLASS_ID    (60)
#define DHCP_OPT_CLIENT_ID          (61)
#define DHCP_OPT_END                (255)

#define PORT_DHCP_SERVER (67)
#define PORT_DHCP_CLIENT (68)

#define DEFAULT_DNS MAKE_IP4(192, 168, 4, 1)
#define DEFAULT_LEASE_TIME_S (24 * 60 * 60) // in seconds

#define MAC_LEN (6)
#define MAKE_IP4(a, b, c, d) ((a) << 24 | (b) << 16 | (c) << 8 | (d))

// clang-format on

typedef struct {
    u8 op;    // message opcode
    u8 htype; // hardware address type
    u8 hlen;  // hardware address length
    u8 hops;
    u32 xid;  // transaction id, chosen by client
    u16 secs; // client seconds elapsed
    u16 flags;
    u8 ciaddr[4];    // client IP address
    u8 yiaddr[4];    // your IP address
    u8 siaddr[4];    // next server IP address
    u8 giaddr[4];    // relay agent IP address
    u8 chaddr[16];   // client hardware address
    u8 sname[64];    // server host name
    u8 file[128];    // boot file name
    u8 options[312]; // optional parameters, variable, starts with magic
} dhcp_msg_t;

static int dhcp_socket_new_dgram(struct udp_pcb **udp, void *cb_data, udp_recv_fn cb_udp_recv) {
    // family is AF_INET
    // type is SOCK_DGRAM

    *udp = udp_new();
    if (*udp == NULL) {
        return -ENOMEM;
    }

    // Register callback
    udp_recv(*udp, cb_udp_recv, (void *)cb_data);

    return 0; // success
}

static void dhcp_socket_free(struct udp_pcb **udp) {
    if (*udp != NULL) {
        udp_remove(*udp);
        *udp = NULL;
    }
}

static int dhcp_socket_bind(struct udp_pcb **udp, u32 ip, u16 port) {
    ip_addr_t addr;
    IP_ADDR4(&addr, ip >> 24 & 0xff, ip >> 16 & 0xff, ip >> 8 & 0xff, ip & 0xff);
    return udp_bind(*udp, &addr, port);
}

static int dhcp_socket_sendto(struct udp_pcb **udp, struct netif *netif, const void *buf, size_t len, u32 ip, u16 port) {
    if (len > 0xffff) {
        len = 0xffff;
    }

    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
    if (p == NULL) {
        return -ENOMEM;
    }

    memcpy(p->payload, buf, len);

    ip_addr_t dest;
    IP_ADDR4(&dest, ip >> 24 & 0xff, ip >> 16 & 0xff, ip >> 8 & 0xff, ip & 0xff);
    err_t err;
    if (netif != NULL) {
        err = udp_sendto_if(*udp, p, &dest, port, netif);
    } else {
        err = udp_sendto(*udp, p, &dest, port);
    }

    pbuf_free(p);

    if (err != ERR_OK) {
        return err;
    }

    return len;
}

static u8 *opt_find(u8 *opt, u8 cmd) {
    for (u32 i = 0; i < 308 && opt[i] != DHCP_OPT_END;) {
        if (opt[i] == cmd) {
            return &opt[i];
        }
        i += 2 + opt[i + 1];
    }
    return NULL;
}

static void opt_write_n(u8 **opt, u8 cmd, size_t n, const void *data) {
    u8 *o = *opt;
    *o++ = cmd;
    *o++ = n;
    memcpy(o, data, n);
    *opt = o + n;
}

static void opt_write_u8(u8 **opt, u8 cmd, u8 val) {
    u8 *o = *opt;
    *o++ = cmd;
    *o++ = 1;
    *o++ = val;
    *opt = o;
}

static void opt_write_u32(u8 **opt, u8 cmd, u32 val) {
    u8 *o = *opt;
    *o++ = cmd;
    *o++ = 4;
    *o++ = val >> 24;
    *o++ = val >> 16;
    *o++ = val >> 8;
    *o++ = val;
    *opt = o;
}

static void dhcp_server_process(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *src_addr, u16_t src_port) {
    DHCPServer *d = arg;
    (void)upcb;
    (void)src_addr;
    (void)src_port;

    // This is around 548 bytes
    dhcp_msg_t dhcp_msg;

    #define DHCP_MIN_SIZE (240 + 3)
    if (p->tot_len < DHCP_MIN_SIZE) {
        goto ignore_request;
    }

    size_t len = pbuf_copy_partial(p, &dhcp_msg, sizeof(dhcp_msg), 0);
    if (len < DHCP_MIN_SIZE) {
        goto ignore_request;
    }

    dhcp_msg.op = DHCPOFFER;
    memcpy(&dhcp_msg.yiaddr, &ip_2_ip4(&d->ip)->addr, 4);

    u8 *opt = (u8 *)&dhcp_msg.options;
    opt += 4; // assume magic cookie: 99, 130, 83, 99

    switch (opt[2]) {
        case DHCPDISCOVER: {
            u32 yi = DHCPS_MAX_IP;
            for (u32 i = 0; i < DHCPS_MAX_IP; ++i) {
                if (memcmp(d->lease[i].mac, dhcp_msg.chaddr, MAC_LEN) == 0) {
                    // MAC match, use this IP address
                    yi = i;
                    break;
                }
                if (yi == DHCPS_MAX_IP) {
                    // Look for a free IP address
                    if (memcmp(d->lease[i].mac, "\x00\x00\x00\x00\x00\x00", MAC_LEN) == 0) {
                        // IP available
                        yi = i;
                    }
                    u32 expiry = d->lease[i].expiry << 16 | 0xffff;
                    if ((i32)(expiry - time_ms()) < 0) {
                        // IP expired, reuse it
                        memset(d->lease[i].mac, 0, MAC_LEN);
                        yi = i;
                    }
                }
            }
            if (yi == DHCPS_MAX_IP) {
                // No more IP addresses left
                goto ignore_request;
            }
            dhcp_msg.yiaddr[3] = DHCPS_BASE_IP + yi;
            opt_write_u8(&opt, DHCP_OPT_MSG_TYPE, DHCPOFFER);
            break;
        }

        case DHCPREQUEST: {
            u8 *o = opt_find(opt, DHCP_OPT_REQUESTED_IP);
            if (o == NULL) {
                // Should be NACK
                goto ignore_request;
            }
            if (memcmp(o + 2, &ip_2_ip4(&d->ip)->addr, 3) != 0) {
                // Should be NACK
                goto ignore_request;
            }
            u8 yi = o[5] - DHCPS_BASE_IP;
            if (yi >= DHCPS_MAX_IP) {
                // Should be NACK
                goto ignore_request;
            }
            if (memcmp(d->lease[yi].mac, dhcp_msg.chaddr, MAC_LEN) == 0) {
                // MAC match, ok to use this IP address
            } else if (memcmp(d->lease[yi].mac, "\x00\x00\x00\x00\x00\x00", MAC_LEN) == 0) {
                // IP unused, ok to use this IP address
                memcpy(d->lease[yi].mac, dhcp_msg.chaddr, MAC_LEN);
            } else {
                // IP already in use
                // Should be NACK
                goto ignore_request;
            }
            d->lease[yi].expiry = (time_ms() + DEFAULT_LEASE_TIME_S * 1000) >> 16;
            dhcp_msg.yiaddr[3] = DHCPS_BASE_IP + yi;
            opt_write_u8(&opt, DHCP_OPT_MSG_TYPE, DHCPACK);
            LWIP_DEBUGF(DHCP_DEBUG,
                        ("DHCPS: client connected: MAC=%02x:%02x:%02x:%02x:%02x:%02x IP=%u.%u.%u.%u\n", dhcp_msg.chaddr[0],
                         dhcp_msg.chaddr[1], dhcp_msg.chaddr[2], dhcp_msg.chaddr[3], dhcp_msg.chaddr[4], dhcp_msg.chaddr[5],
                         dhcp_msg.yiaddr[0], dhcp_msg.yiaddr[1], dhcp_msg.yiaddr[2], dhcp_msg.yiaddr[3]));
            break;
        }

        default:
            goto ignore_request;
    }

    opt_write_n(&opt, DHCP_OPT_SERVER_ID, 4, &ip_2_ip4(&d->ip)->addr);
    opt_write_n(&opt, DHCP_OPT_SUBNET_MASK, 4, &ip_2_ip4(&d->nm)->addr);
    opt_write_n(&opt, DHCP_OPT_ROUTER, 4, &ip_2_ip4(&d->ip)->addr); // aka gateway; can have multiple addresses
    opt_write_u32(&opt, DHCP_OPT_DNS, DEFAULT_DNS);                 // can have multiple addresses
    opt_write_u32(&opt, DHCP_OPT_IP_LEASE_TIME, DEFAULT_LEASE_TIME_S);
    *opt++ = DHCP_OPT_END;
    struct netif *netif = ip_current_input_netif();
    dhcp_socket_sendto(&d->udp, netif, &dhcp_msg, opt - (u8 *)&dhcp_msg, 0xffffffff, PORT_DHCP_CLIENT);

ignore_request:
    pbuf_free(p);
}

bool dhcp_server_init(DHCPServer *d, ip_addr_t *ip, ip_addr_t *nm) {
    ip_addr_copy(d->ip, *ip);
    ip_addr_copy(d->nm, *nm);
    memset(d->lease, 0, sizeof(d->lease));
    if (dhcp_socket_new_dgram(&d->udp, d, dhcp_server_process) != 0)
        return false;
    return (dhcp_socket_bind(&d->udp, 0, PORT_DHCP_SERVER) == 0);
}

void dhcp_server_deinit(DHCPServer *d) {
    dhcp_socket_free(&d->udp);
}

#endif // PLATFORM_SUPPORTS_WIFI
