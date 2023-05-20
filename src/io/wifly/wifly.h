#ifndef wifly_h
#define wifly_h

// Print misc debug messages to serial during Wi-Fly wireless operations
#define DHCP_DEBUG_printf // printf
#define DNS_DEBUG_printf // printf
#define TCP_DEBUG_printf // printf
#define ERROR_printf // printf
#define DUMP_DATA 0 // 1

// Content that is displayed to user, formatted as HTML
#define PAGE_CONTENT "<html><head><meta name=\"viewport\" content=\"width=device-width\"><title>Wi-Fly</title><style>body{background-color:black;}.text{color:white;font-family:sans-serif;}#fplan{width:350px;height:500px;}</style></head><body><h2 class=text>Wi-Fly</h2><form><label class=text for=fplan>Paste flightplan here:</label><br><input type=text id=fplan name=fplan><br><br><input type=submit value=\"Upload flightplan\"></form></body></html>"

/**
 * Initializes the Wi-Fly system.
*/
void wifly_init();

/**
 * De-initializes the Wi-Fly system.
*/
void wifly_deinit();

#endif // wifly_h
