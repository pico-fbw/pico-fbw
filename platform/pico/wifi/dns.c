/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * This file utilizes code under the BSD-3-Clause License. See "LICENSE" for details.
 */

// Big thanks to Raspberry Pi (as part of the pico-examples) for the DNS server implementation on the Pico W!

/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "dns.h"

#if PLATFORM_SUPPORTS_WIFI

// clang-format off

#include <errno.h>
#include <string.h>
#include "platform/int.h"

#include "lwip/debug.h"
#include "lwip/udp.h"

#define PORT_DNS_SERVER 53
#define MAX_DNS_MSG_SIZE 300

// clang-format on

typedef struct dns_header_t_ {
    u16 id;
    u16 flags;
    u16 question_count;
    u16 answer_record_count;
    u16 authority_record_count;
    u16 additional_record_count;
} dns_header_t;

static int dns_socket_new_dgram(struct udp_pcb **udp, void *cb_data, udp_recv_fn cb_udp_recv) {
    *udp = udp_new();
    if (*udp == NULL) {
        return -ENOMEM;
    }
    udp_recv(*udp, cb_udp_recv, (void *)cb_data);
    return ERR_OK;
}

static void dns_socket_free(struct udp_pcb **udp) {
    if (*udp != NULL) {
        udp_remove(*udp);
        *udp = NULL;
    }
}

static int dns_socket_bind(struct udp_pcb **udp, u32 ip, u16 port) {
    ip_addr_t addr;
    IP4_ADDR(&addr, ip >> 24 & 0xff, ip >> 16 & 0xff, ip >> 8 & 0xff, ip & 0xff);
    err_t err = udp_bind(*udp, &addr, port);
    if (err != ERR_OK) {
        LWIP_DEBUGF(DNS_DEBUG, ("dns failed to bind to port %u: %d", port, err));
    }
    return err;
}

static int dns_socket_sendto(struct udp_pcb **udp, const void *buf, size_t len, const ip_addr_t *dest, u16 port) {
    if (len > 0xffff) {
        len = 0xffff;
    }

    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
    if (p == NULL) {
        LWIP_DEBUGF(DNS_DEBUG, ("DNS: Failed to send message out of memory\n"));
        return -ENOMEM;
    }

    memcpy(p->payload, buf, len);
    err_t err = udp_sendto(*udp, p, dest, port);

    pbuf_free(p);

    if (err != ERR_OK) {
        LWIP_DEBUGF(DNS_DEBUG, ("DNS: Failed to send message %d\n", err));
        return err;
    }

    return len;
}

static void dns_server_process(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *src_addr, u16_t src_port) {
    DNSServer *d = arg;
    LWIP_DEBUGF(DNS_DEBUG, ("dns_server_process %u\n", p->tot_len));

    u8 dns_msg[MAX_DNS_MSG_SIZE];
    dns_header_t *dns_hdr = (dns_header_t *)dns_msg;

    size_t msg_len = pbuf_copy_partial(p, dns_msg, sizeof(dns_msg), 0);
    if (msg_len < sizeof(dns_header_t)) {
        goto ignore_request;
    }

    u16 flags = lwip_ntohs(dns_hdr->flags);
    u16 question_count = lwip_ntohs(dns_hdr->question_count);

    LWIP_DEBUGF(DNS_DEBUG, ("len %d\n", msg_len));
    LWIP_DEBUGF(DNS_DEBUG, ("dns flags 0x%x\n", flags));
    LWIP_DEBUGF(DNS_DEBUG, ("dns question count 0x%x\n", question_count));

    // flags from rfc1035
    // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    // |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
    // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

    // Check QR indicates a query
    if (((flags >> 15) & 0x1) != 0) {
        LWIP_DEBUGF(DNS_DEBUG, ("Ignoring non-query\n"));
        goto ignore_request;
    }

    // Check for standard query
    if (((flags >> 11) & 0xf) != 0) {
        LWIP_DEBUGF(DNS_DEBUG, ("Ignoring non-standard query\n"));
        goto ignore_request;
    }

    // Check question count
    if (question_count < 1) {
        LWIP_DEBUGF(DNS_DEBUG, ("Invalid question count\n"));
        goto ignore_request;
    }

    // Print the question
    LWIP_DEBUGF(DNS_DEBUG, ("question: "));
    const u8 *question_ptr_start = dns_msg + sizeof(dns_header_t);
    const u8 *question_ptr_end = dns_msg + msg_len;
    const u8 *question_ptr = question_ptr_start;
    while (question_ptr < question_ptr_end) {
        if (*question_ptr == 0) {
            question_ptr++;
            break;
        } else {
            if (question_ptr > question_ptr_start) {
                LWIP_DEBUGF(DNS_DEBUG, ("."));
            }
            u16 label_len = *question_ptr++;
            if (label_len > 63) {
                LWIP_DEBUGF(DNS_DEBUG, ("Invalid label\n"));
                goto ignore_request;
            }
            LWIP_DEBUGF(DNS_DEBUG, ("%.*s", label_len, question_ptr));
            question_ptr += label_len;
        }
    }
    LWIP_DEBUGF(DNS_DEBUG, ("\n"));

    // Check question length
    if (question_ptr - question_ptr_start > 255) {
        LWIP_DEBUGF(DNS_DEBUG, ("Invalid question length\n"));
        goto ignore_request;
    }

    // Skip QNAME and QTYPE
    question_ptr += 4;

    // Generate answer
    u8 *answer_ptr = dns_msg + (question_ptr - dns_msg);
    *answer_ptr++ = 0xc0;                         // pointer
    *answer_ptr++ = question_ptr_start - dns_msg; // pointer to question

    *answer_ptr++ = 0;
    *answer_ptr++ = 1; // host address

    *answer_ptr++ = 0;
    *answer_ptr++ = 1; // Internet class

    *answer_ptr++ = 0;
    *answer_ptr++ = 0;
    *answer_ptr++ = 0;
    *answer_ptr++ = 60; // ttl 60s

    *answer_ptr++ = 0;
    *answer_ptr++ = 4;                  // length
    memcpy(answer_ptr, &d->ip.addr, 4); // use our address
    answer_ptr += 4;

    dns_hdr->flags = lwip_htons(0x1 << 15 | // QR = response
                                0x1 << 10 | // AA = authoritive
                                0x1 << 7);  // RA = authenticated
    dns_hdr->question_count = lwip_htons(1);
    dns_hdr->answer_record_count = lwip_htons(1);
    dns_hdr->authority_record_count = 0;
    dns_hdr->additional_record_count = 0;

    // Send the reply
    LWIP_DEBUGF(DNS_DEBUG, ("Sending %d byte reply to %s:%d\n", answer_ptr - dns_msg, ipaddr_ntoa(src_addr), src_port));
    dns_socket_sendto(&d->udp, &dns_msg, answer_ptr - dns_msg, src_addr, src_port);

ignore_request:
    pbuf_free(p);
    (void)upcb;
}

bool dns_server_init(DNSServer *d, ip_addr_t *ip) {
    if (dns_socket_new_dgram(&d->udp, d, dns_server_process) != ERR_OK) {
        LWIP_DEBUGF(DNS_DEBUG, ("dns server failed to start\n"));
        return false;
    }
    if (dns_socket_bind(&d->udp, 0, PORT_DNS_SERVER) != ERR_OK) {
        LWIP_DEBUGF(DNS_DEBUG, ("dns server failed to bind\n"));
        return false;
    }
    ip_addr_copy(d->ip, *ip);
    LWIP_DEBUGF(DNS_DEBUG, ("dns server listening on port %d\n", PORT_DNS_SERVER));
    return true;
}

void dns_server_deinit(DNSServer *d) {
    dns_socket_free(&d->udp);
}

#endif // PLATFORM_SUPPORTS_WIFI
