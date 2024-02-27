/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 * 
 * This file utilizes code under the BSD-3-Clause License. See "LICENSE" for details.
*/

/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
*/

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include "platform/int.h"

#include "lwip/udp.h"

#include "sys/print.h"

#include "wifly/wifly.h"

#include "dns.h"

#define PORT_DNS_SERVER 53

typedef struct dns_header_t_ {
    u16 id;
    u16 flags;
    u16 question_count;
    u16 answer_record_count;
    u16 authority_record_count;
    u16 additional_record_count;
} dns_header_t;

#define MAX_DNS_MSG_SIZE 300

static i32 dns_socket_new_dgram(struct udp_pcb **udp, void *cb_data, udp_recv_fn cb_udp_recv) {
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

static i32 dns_socket_bind(struct udp_pcb **udp, u32 ip, u16 port) {
    ip_addr_t addr;
    IP4_ADDR(&addr, ip >> 24 & 0xff, ip >> 16 & 0xff, ip >> 8 & 0xff, ip & 0xff);
    err_t err = udp_bind(*udp, &addr, port);
    if (err != ERR_OK) {
        printfbw(network, "[dns] ERROR: failed to bind to port %u: %d", port, err);
        assert(false);
    }
    return err;
}

static void dump_bytes(const u8 *bptr, u32 len) {
    unsigned i32 i = 0;

    for (i = 0; i < len;) {
        if ((i & 0x0f) == 0) {
            printraw("\n");
        } else if ((i & 0x07) == 0) {
            printraw(" ");
        }
        printraw("%02x ", bptr[i++]);
    }
    printraw("\n");
}

static i32 dns_socket_sendto(struct udp_pcb **udp, const void *buf, size_t len, const ip_addr_t *dest, u16 port) {
    if (len > 0xffff) {
        len = 0xffff;
    }

    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
    if (p == NULL) {
        printfbw(network, "[dns] ERROR: failed to allocate memory!");
        return -ENOMEM;
    }

    memcpy(p->payload, buf, len);
    err_t err = udp_sendto(*udp, p, dest, port);

    pbuf_free(p);

    if (err != ERR_OK) {
        printfbw(network, "[dns] ERROR: failed to send message %d", err);
        return err;
    }

    if (debug.dumpNetwork) dump_bytes(buf, len);

    return len;
}

static void dns_server_process(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *src_addr, u16_t src_port) {
    dns_server_t *d = arg;
    printfbw(network, "[dns] server processing %u", p->tot_len);

    u8 dns_msg[MAX_DNS_MSG_SIZE];
    dns_header_t *dns_hdr = (dns_header_t*)dns_msg;

    size_t msg_len = pbuf_copy_partial(p, dns_msg, sizeof(dns_msg), 0);
    if (msg_len < sizeof(dns_header_t)) {
        goto ignore_request;
    }

    if (shouldPrint.network) dump_bytes(dns_msg, msg_len);

    u16 flags = lwip_ntohs(dns_hdr->flags);
    u16 question_count = lwip_ntohs(dns_hdr->question_count);

    printfbw(network, "[dns] len %d", msg_len);
    printfbw(network, "[dns] flags 0x%x", flags);
    printfbw(network, "[dns] question count 0x%x", question_count);

    // flags from rfc1035
    // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    // |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
    // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

    // Check QR indicates a query
    if (((flags >> 15) & 0x1) != 0) {
        printfbw(network, "[dns] WARNING: ignoring non-query");
        goto ignore_request;
    }

    // Check for standard query
    if (((flags >> 11) & 0xf) != 0) {
        printfbw(network, "[dns] WARNING: ignoring non-standard query");
        goto ignore_request;
    }

    // Check question count
    if (question_count < 1) {
        printfbw(network, "[dns] WARNING: invalid question count");
        goto ignore_request;
    }

    // Print the question
    if (shouldPrint.network) printraw("[dns] question: ");
    const u8 *question_ptr_start = dns_msg + sizeof(dns_header_t);
    const u8 *question_ptr_end = dns_msg + msg_len;
    const u8 *question_ptr = question_ptr_start;
    while(question_ptr < question_ptr_end) {
        if (*question_ptr == 0) {
            question_ptr++;
            break;
        } else {
            if (question_ptr > question_ptr_start) {
                if (shouldPrint.network) printraw(".");
            }
            i32 label_len = *question_ptr++;
            if (label_len > 63) {
                if (shouldPrint.network) printraw("invalid label\n");
                goto ignore_request;
            }
            if (shouldPrint.network) printraw("%.*s", label_len, question_ptr);
            question_ptr += label_len;
        }
    }
    if (shouldPrint.network) printraw("\n");

    // Check question length
    if (question_ptr - question_ptr_start > 255) {
        printfbw(network, "[dns] WARNING: invalid question length");
        goto ignore_request;
    }

    // Skip QNAME and QTYPE
    question_ptr += 4;

    // Generate answer
    u8 *answer_ptr = dns_msg + (question_ptr - dns_msg);
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
    printfbw(network, "[dns] sending %d byte reply to %s:%d", answer_ptr - dns_msg, ipaddr_ntoa(src_addr), src_port);
    dns_socket_sendto(&d->udp, &dns_msg, answer_ptr - dns_msg, src_addr, src_port);

ignore_request:
    pbuf_free(p);
}

void dns_server_init(dns_server_t *d, ip_addr_t *ip) {
    if (dns_socket_new_dgram(&d->udp, d, dns_server_process) != ERR_OK) {
        printfbw(network, "[dns] ERROR: server failed to start");
        return;
    }
    if (dns_socket_bind(&d->udp, 0, PORT_DNS_SERVER) != ERR_OK) {
        printfbw(network, "[dns] ERROR: server failed to bind");
        return;
    }
    ip_addr_copy(d->ip, *ip);
    printfbw(network, "[dns] server listening on port %d", PORT_DNS_SERVER);
}

void dns_server_deinit(dns_server_t *d) {
    dns_socket_free(&d->udp);
}
