#ifndef __DNS_H
#define __DNS_H

#include "lwip/ip_addr.h"

typedef struct dns_server_t_ {
    struct udp_pcb *udp;
     ip_addr_t ip;
} dns_server_t;

/**
 * Initializes the DNS server.
 * @param dns_server_t the DNS server to initialize
 * @param ip_addr_t the IP address to bind to
*/
void dns_server_init(dns_server_t *d, ip_addr_t *ip);

/**
 * Deinitializes the DNS server.
 * @param dns_server_t the DNS server to deinitialize
*/
void dns_server_deinit(dns_server_t *d);

#endif // __DNS_H
