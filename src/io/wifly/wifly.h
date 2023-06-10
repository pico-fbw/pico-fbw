#ifndef wifly_h
#define wifly_h

// Print misc debug messages to serial during Wi-Fly wireless operations
#define WIFLY_DEBUG_printf  // printf
#define DHCP_DEBUG_printf   // printf
#define DNS_DEBUG_printf    // printf
#define TCP_DEBUG_printf    // printf
#define ERROR_printf        // printf
#define DUMP_DATA 0         // 1
#define TCP_DUMP_DATA 0     // 1
#define WIFLY_DUMP_DATA 0   // 1

#define WIFLY_STATUS_AWAITING -1
#define WIFLY_STATUS_OK 0
#define WIFLY_ERROR_PARSE 1
#define WIFLY_ERROR_VERSION 2
#define WIFLY_ERROR_MEM 3

// Content that is displayed to user, formatted as HTML
#define PAGE_CONTENT "<!DOCTYPE html><html lang=en><head><meta name=viewport content=\"width=device-width\"><title>Wi-Fly</title><style>body{background-color:black;padding:0px 10px;font-family:sans-serif;margin:auto;width:92%%;}.header{display:flex;align-items:center;}.icon{margin:15px 8px 15px 0px;}.text{color:white;}#fplan{width:99.6%%;height:250px;}#upload{background-color:%s;border-radius:8px;padding:8px 15px;text-align:center;font-size:16px;}</style></head><body><div class=header><h2 class=text>Wi-Fly</h2></div><form><textarea id=fplan name=fplan placeholder=\"Paste your flightplan here...\"></textarea><br><br><input type=submit class=text id=upload value=Upload></form><p class=text>%s</p></body></html>"
// TODO: use accumulated headers for sending packets as well so I can send the wifly image as well: <img class=icon src=data:image/png;base64,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>
// HTTP parameter that defines flightplan in GET request
#define FPLAN_PARAM "fplan="

typedef struct Waypoint {
    double lat;
    double lng;
    int_fast16_t alt;
} Waypoint;

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

/**
 * Gets the current flightplan (structured as a list of Waypoints).
 * @return A pointer to the current flightplan, or NULL if there is none.
*/
Waypoint *wifly_getFplan();

#endif // wifly_h
