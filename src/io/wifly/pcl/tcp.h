#ifndef __TCP_H
#define __TCP_H

#include "../wifly.h"

typedef struct TCP_SERVER_T_ {
    struct tcp_pcb *server_pcb;
    bool complete;
    ip_addr_t gw;
} TCP_SERVER_T;

typedef struct TCP_CONNECT_STATE_T_ {
    struct tcp_pcb *pcb;
    int sent_len;
    char headers[TCP_HEADER_SIZE];
    char* accHeaders;
    char result[TCP_RESULT_SIZE];
    int header_len;
    int result_len;
    ip_addr_t *gw;
} TCP_CONNECT_STATE_T;

/**
 * Opens the TCP server.
 * @param arg The state object to open
*/
bool tcp_server_open(void *arg);

/**
 * Closes the TCP server.
 * @param state The state object to close
*/
void tcp_server_close(TCP_SERVER_T *state);

#endif // __TCP_H