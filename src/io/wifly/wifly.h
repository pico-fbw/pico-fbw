#ifndef wifly_h
#define wifly_h

// Current supported Wi-Fly standard. If the server recieves a versioncode other than this, it will not accept the flightplan.
#define WIFLY_CURRENT_VERSION "0.1a"

// Print misc debug messages to serial during Wi-Fly wireless operations
#define WIFLY_DEBUG_printf // printf
#define DHCP_DEBUG_printf // printf
#define DNS_DEBUG_printf // printf
#define TCP_DEBUG_printf // printf
#define ERROR_printf // printf
#define DUMP_DATA 0 // 1
#define TCP_DUMP_DATA 0 // 1

// Content that is displayed to user, formatted as HTML
#define PAGE_CONTENT "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width\"><title>Wi-Fly</title><style>body{background-color:black;padding:0px 10px;font-family:sans-serif;margin:auto;width:94%%;}.text{color:white;}#fplan{width:100%%;height:500px;}#upload{background-color:%s;padding:8px 15px;text-align:center;font-size:16px;}</style></head><body><h2 class=text>Wi-Fly</h2><form><input type=text id=fplan name=fplan placeholder=\"Paste your flightplan here...\"><br><br><input type=submit class=text id=upload value=Upload></form><p class=text>%s</p></body></html>"
// HTTP parameter that defines flightplan in GET request
#define FPLAN_PARAM "fplan="

/**
 * Initializes the Wi-Fly system.
*/
void wifly_init();

/**
 * De-initializes the Wi-Fly system.
*/
void wifly_deinit();

/**
 * Parses the Wi-Fly flightplan.
 * @param fplan The flightplan to parse.
 * @return The outcome of parsing. 0 if successful, 1 if failure (syntax error), 2 if failure (version mismatch).
*/
int wifly_parseFplan(const char *fplan);

#endif // wifly_h
