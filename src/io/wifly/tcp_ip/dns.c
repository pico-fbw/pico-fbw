/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "lwip/udp.h"

#include "../wifly.h"

#include "../../../config.h"

#include "dns.h"

#define PORT_DNS_SERVER 53

typedef struct dns_header_t_ {
    uint16_t id;
    uint16_t flags;
    uint16_t question_count;
    uint16_t answer_record_count;
    uint16_t authority_record_count;
    uint16_t additional_record_count;
} dns_header_t;

#define MAX_DNS_MSG_SIZE 300

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

static int dns_socket_bind(struct udp_pcb **udp, uint32_t ip, uint16_t port) {
    ip_addr_t addr;
    IP4_ADDR(&addr, ip >> 24 & 0xff, ip >> 16 & 0xff, ip >> 8 & 0xff, ip & 0xff);
    err_t err = udp_bind(*udp, &addr, port);
    if (err != ERR_OK) {
        FBW_DEBUG_printf("[dns] ERROR: failed to bind to port %u: %d", port, err);
        assert(false);
    }
    return err;
}

#if DNS_DUMP_DATA
    static void dump_bytes(const uint8_t *bptr, uint32_t len) {
        unsigned int i = 0;

        for (i = 0; i < len;) {
            if ((i & 0x0f) == 0) {
                printf("\n");
            } else if ((i & 0x07) == 0) {
                printf(" ");
            }
            printf("%02x ", bptr[i++]);
        }
        printf("\n");
    }
#endif

static int dns_socket_sendto(struct udp_pcb **udp, const void *buf, size_t len, const ip_addr_t *dest, uint16_t port) {
    if (len > 0xffff) {
        len = 0xffff;
    }

    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
    if (p == NULL) {
        FBW_DEBUG_printf("[dns] ERROR: failed to allocate memory\n");
        return -ENOMEM;
    }

    memcpy(p->payload, buf, len);
    err_t err = udp_sendto(*udp, p, dest, port);

    pbuf_free(p);

    if (err != ERR_OK) {
        FBW_DEBUG_printf("[dns] ERROR: failed to send message %d\n", err);
        return err;
    }

    #if DNS_DUMP_DATA
        dump_bytes(buf, len);
    #endif

    return len;
}

static void dns_server_process(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *src_addr, u16_t src_port) {
    dns_server_t *d = arg;
    DNS_DEBUG_printf("[dns] server processing %u\n", p->tot_len);

    uint8_t dns_msg[MAX_DNS_MSG_SIZE];
    dns_header_t *dns_hdr = (dns_header_t*)dns_msg;

    size_t msg_len = pbuf_copy_partial(p, dns_msg, sizeof(dns_msg), 0);
    if (msg_len < sizeof(dns_header_t)) {
        goto ignore_request;
    }

    #if DNS_DUMP_DATA
        dump_bytes(dns_msg, msg_len);
    #endif

    uint16_t flags = lwip_ntohs(dns_hdr->flags);
    uint16_t question_count = lwip_ntohs(dns_hdr->question_count);

    DNS_DEBUG_printf("[dns] len %d\n", msg_len);
    DNS_DEBUG_printf("[dns] flags 0x%x\n", flags);
    DNS_DEBUG_printf("[dns] question count 0x%x\n", question_count);

    // flags from rfc1035
    // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    // |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
    // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

    // Check QR indicates a query
    if (((flags >> 15) & 0x1) != 0) {
        FBW_DEBUG_printf("[dns] WARNING: ignoring non-query\n");
        goto ignore_request;
    }

    // Check for standard query
    if (((flags >> 11) & 0xf) != 0) {
        FBW_DEBUG_printf("[dns] WARNING: ignoring non-standard query\n");
        goto ignore_request;
    }

    // Check question count
    if (question_count < 1) {
        FBW_DEBUG_printf("[dns] WARNING: invalid question count\n");
        goto ignore_request;
    }

    // Print the question
    DNS_DEBUG_printf("[dns] question: ");
    const uint8_t *question_ptr_start = dns_msg + sizeof(dns_header_t);
    const uint8_t *question_ptr_end = dns_msg + msg_len;
    const uint8_t *question_ptr = question_ptr_start;
    while(question_ptr < question_ptr_end) {
        if (*question_ptr == 0) {
            question_ptr++;
            break;
        } else {
            if (question_ptr > question_ptr_start) {
                DNS_DEBUG_printf(".");
            }
            int label_len = *question_ptr++;
            if (label_len > 63) {
                DNS_DEBUG_printf("invalid label\n");
                goto ignore_request;
            }
            DNS_DEBUG_printf("%.*s", label_len, question_ptr);
            question_ptr += label_len;
        }
    }
    DNS_DEBUG_printf("\n");

    // Check question length
    if (question_ptr - question_ptr_start > 255) {
        FBW_DEBUG_printf("[dns] WARNING: invalid question length\n");
        goto ignore_request;
    }

    // Skip QNAME and QTYPE
    question_ptr += 4;

    // Generate answer
    uint8_t *answer_ptr = dns_msg + (question_ptr - dns_msg);
    *answer_ptr++ = 0xc0; // pointer
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
    *answer_ptr++ = 4; // length
    memcpy(answer_ptr, &d->ip.addr, 4); // use our address
    answer_ptr += 4;

    dns_hdr->flags = lwip_htons(
                0x1 << 15 | // QR = response
                0x1 << 10 | // AA = authoritive
                0x1 << 7);   // RA = authenticated
    dns_hdr->question_count = lwip_htons(1);
    dns_hdr->answer_record_count = lwip_htons(1);
    dns_hdr->authority_record_count = 0;
    dns_hdr->additional_record_count = 0;

    // Send the reply
    DNS_DEBUG_printf("[dns] sending %d byte reply to %s:%d\n", answer_ptr - dns_msg, ipaddr_ntoa(src_addr), src_port);
    dns_socket_sendto(&d->udp, &dns_msg, answer_ptr - dns_msg, src_addr, src_port);

ignore_request:
    pbuf_free(p);
}

void dns_server_init(dns_server_t *d, ip_addr_t *ip) {
    if (dns_socket_new_dgram(&d->udp, d, dns_server_process) != ERR_OK) {
        FBW_DEBUG_printf("[dns] ERROR: server failed to start\n");
        return;
    }
    if (dns_socket_bind(&d->udp, 0, PORT_DNS_SERVER) != ERR_OK) {
        FBW_DEBUG_printf("[dns] ERROR: server failed to bind\n");
        return;
    }
    ip_addr_copy(d->ip, *ip);
    DNS_DEBUG_printf("[dns] server listening on port %d\n", PORT_DNS_SERVER);
}

void dns_server_deinit(dns_server_t *d) {
    dns_socket_free(&d->udp);
}
