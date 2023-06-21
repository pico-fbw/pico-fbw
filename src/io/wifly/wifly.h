#ifndef wifly_h
#define wifly_h

// Status codes for after a flightplan has been submitted
#define WIFLY_STATUS_AWAITING -1
#define WIFLY_STATUS_OK 0
#define WIFLY_ERROR_PARSE 1
#define WIFLY_ERROR_VERSION 2
#define WIFLY_ERROR_MEM 3
#define WIFLY_ERROR_FW_VERSION 4

// Color codes that are displayed to the user for feedback
#define WIFLY_HEX_OK "#4CAF50"
#define WIFLY_HEX_WARN "#A6710C"
#define WIFLY_HEX_ERR "#D21404"
#define WIFLY_HEX_INACTIVE "#5A5A5A"

// Content that is displayed to user, formatted as HTML
#define PAGE_CONTENT "<!DOCTYPE html><html lang=en><head><meta name=viewport content=\"width=device-width\"><title>Wi-Fly</title><style>body{background-color:black;padding:0px 10px;font-family:sans-serif;margin:auto;width:92%%;}.header{display:flex;align-items:center;}.icon{margin:15px 8px 15px 0px;}.text{color:white;}#fplan{width:99.6%%;height:250px;}#upload{background-color:%s;border-radius:8px;padding:8px 15px;text-align:center;font-size:16px;}</style></head><body><div class=header><h2 class=text>Wi-Fly</h2></div><form><textarea id=fplan name=fplan placeholder=\"Paste your flightplan here...\"></textarea><br><br><input type=submit class=text id=upload value=Upload></form><p class=text>%s</p></body></html>"
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
 * Generates the page content for Wi-Fly based on the current status of the flightplan.
 * @param result Pointer to the result buffer.
 * @param max_result_len The maximum length of the result buffer.
 * @param status The current status of the flightplan.
*/
int wifly_genPageContent(char *result, size_t max_result_len, int status);

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

/**
 * @return the number of waypoints in the current flightplan (0 if a flightplan has not yet been parsed).
*/
uint wifly_getWaypointCount();

#endif // wifly_h
