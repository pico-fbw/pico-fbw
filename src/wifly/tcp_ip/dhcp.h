#ifndef __DHCP_H
#define __DHCP_H

#ifndef MICROPY_INCLUDED_LIB_NETUTILS_DHCPSERVER_H
    #define MICROPY_INCLUDED_LIB_NETUTILS_DHCPSERVER_H

    #include "lwip/ip_addr.h"

    #define DHCPS_BASE_IP (16)
    #define DHCPS_MAX_IP (8)

    typedef struct _dhcp_server_lease_t {
        uint8_t mac[6];
        uint16_t expiry;
    } dhcp_server_lease_t;

    typedef struct _dhcp_server_t {
        ip_addr_t ip;
        ip_addr_t nm;
        dhcp_server_lease_t lease[DHCPS_MAX_IP];
        struct udp_pcb *udp;
    } dhcp_server_t;

    /**
     * Initializes the DHCP server.
     * @param dhcp_server_t the DHCP server to initialize.  
     * @param ip_addr_t the IP address to use.
     * @param ip_addr_t the network mask to use.
    */
    void dhcp_server_init(dhcp_server_t *d, ip_addr_t *ip, ip_addr_t *nm);

    /**
     * Deinitializes the DHCP server.
     * @param dhcp_server_t *d the DHCP server to deinitialize.
    */
    void dhcp_server_deinit(dhcp_server_t *d);

#endif // MICROPY_INCLUDED_LIB_NETUTILS_DHCPSERVER_H

#endif // __DHCP_H
