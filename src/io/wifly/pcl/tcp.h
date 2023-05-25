#ifndef tcp_h
#define tcp_h

typedef struct TCP_SERVER_T_ {
    struct tcp_pcb *server_pcb;
    bool complete;
    ip_addr_t gw;
} TCP_SERVER_T;

typedef struct TCP_CONNECT_STATE_T_ {
    struct tcp_pcb *pcb;
    int sent_len;
    char headers[1460];
    char* accHeaders;
    char result[1460];
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

#endif // tcp_h