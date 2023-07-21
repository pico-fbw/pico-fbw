#ifndef __WIFLY_H
#define __WIFLY_H

// Status codes for after a flightplan has been submitted
#define WIFLY_STATUS_AWAITING -1
#define WIFLY_STATUS_OK 0
#define WIFLY_ERROR_PARSE 1
#define WIFLY_ERROR_VERSION 2
#define WIFLY_ERROR_MEM 3
#define WIFLY_ERROR_FW_VERSION 4

// Color codes that are displayed to the user for feedback
#define WIFLY_HEX_OK "#22C55E"
#define WIFLY_HEX_WARN "#F97316"
#define WIFLY_HEX_ERR "#EF4444"
#define WIFLY_HEX_INACTIVE "#6366F1"

// Content that is displayed to user, formatted as HTML
#define PAGE_CONTENT "<!DOCTYPE html><html lang=en><head><meta name=viewport content=\"width=device-width\"><title>Wi-Fly</title><style>body{background-color:#111827;padding:0px 10px;font-family:sans-serif;margin:auto;width:98%%;}.header{display:flex;align-items:center;}.icon{width:100px;height:100px;margin-left:-16px;}.text{color:white;}#fplan{background-color:#1D2432;border-color:#343A47;border-width:1px;width:99.6%%;height:200px;box-sizing:border-box;resize:none;}#upload{background-color:%s;border-radius:4px;border-width:0px;padding:0.5rem 0.75rem 0.5rem 0.75rem;text-align:center;font-size:14px;line-height:20px;font-weight:800;}</style></head><body><div class=header><svg viewBox=\"0 0 24 24\" fill=\"currentColor\" class=icon xmlns=\"http://www.w3.org/2000/svg\"><path style=\"fill:#2563eb;fill-opacity:1\" d=\"M4.875 4.117v.002a.75.75 0 0 0-.75.748v.75a.75.75 0 0 0 .75.75c2.869 0 5.516.948 7.646 2.547a34.04 34.04 0 0 1 .352-.096 34.04 34.04 0 0 1 2.275-.45.422.422 0 0 1 .114-.003.422.422 0 0 1 .09.022 14.95 14.95 0 0 0-10.477-4.27Z\"/><path style=\"fill:#db2777;fill-opacity:1\" d=\"m4.78 13.156 3.574 1.893.457-.457a6 6 0 0 0-3.92-1.46v.003a.75.75 0 0 0-.112.021z\"/><path style=\"fill:#2563eb;fill-opacity:1\" d=\"M4.89 17.633v.002a.75.75 0 0 0-.75.748v.75a.75.75 0 0 0 .75.75h.75a.75.75 0 0 0 .75-.75 1.5 1.5 0 0 0-1.5-1.5zm1.536-6.6a34.04 34.04 0 0 1 2.015-.853 34.04 34.04 0 0 1 1.204-.414 10.444 10.444 0 0 0-4.77-1.149v.002a.75.75 0 0 0-.75.748v.75a.75.75 0 0 0 .75.75 8.25 8.25 0 0 1 1.55.166z\"/><path style=\"fill:#db2777;fill-opacity:1\" d=\"m10.398 13.004 1.342-1.342a.422.422 0 0 1 .233-.119.422.422 0 0 1 .2.031 10.53 10.53 0 0 0-2.528-1.808 34.04 34.04 0 0 0-1.204.414 34.04 34.04 0 0 0-2.015.853 8.25 8.25 0 0 1 3.972 1.97z\"/><path style=\"fill:#2563eb;fill-opacity:1\" d=\"m10.996 13.602 1.342-1.342a.422.422 0 0 0 .119-.233.422.422 0 0 0-.031-.203 10.56 10.56 0 0 0-.252-.25.422.422 0 0 0-.201-.031.422.422 0 0 0-.233.12l-1.342 1.34a8.25 8.25 0 0 1 .598.599z\"/><path style=\"fill:#db2777;fill-opacity:1\" d=\"M10.873 19.28a.75.75 0 0 0 .018-.147 6 6 0 0 0-1.48-3.945l-.46.458z\"/><path style=\"fill:#2563eb;fill-opacity:1\" d=\"m10.873 19.28-1.922-3.634.46-.459a6 6 0 0 0-.6-.595l-.457.457-3.575-1.893a.75.75 0 0 0-.638.727v.75a.75.75 0 0 0 .75.75 3.75 3.75 0 0 1 3.75 3.75.75.75 0 0 0 .75.75h.75a.75.75 0 0 0 .732-.604z\"/><path style=\"fill:#db2777;fill-opacity:1\" d=\"M14.234 14.36a10.53 10.53 0 0 0-1.808-2.536.422.422 0 0 1 .031.203.422.422 0 0 1-.12.233l-1.34 1.342a8.25 8.25 0 0 1 1.966 3.982 34.042 34.042 0 0 0 .857-2.025 34.042 34.042 0 0 0 .414-1.2z\"/><path style=\"fill:#2563eb;fill-opacity:1\" d=\"M14.234 14.36a34.042 34.042 0 0 1-.414 1.199 34.042 34.042 0 0 1-.857 2.025 8.25 8.25 0 0 1 .162 1.533.75.75 0 0 0 .75.75h.75a.75.75 0 0 0 .75-.75c0-1.713-.413-3.329-1.14-4.758zm1.379-5.71a.422.422 0 0 1 .022.088.422.422 0 0 1-.002.114 34.042 34.042 0 0 1-.451 2.275 34.042 34.042 0 0 1-.096.353 12.696 12.696 0 0 1 2.539 7.637.75.75 0 0 0 .75.75h.75a.75.75 0 0 0 .75-.75A14.95 14.95 0 0 0 15.613 8.65z\"/><path style=\"fill:#db2777;fill-opacity:1\" d=\"M15.352 8.387a.422.422 0 0 0-.09-.022.422.422 0 0 0-.114.002 34.04 34.04 0 0 0-2.275.451 34.04 34.04 0 0 0-.352.096c.972.73 1.836 1.594 2.565 2.566a34.042 34.042 0 0 0 .096-.353 34.042 34.042 0 0 0 .45-2.275.422.422 0 0 0 .003-.114.422.422 0 0 0-.022-.088 15.244 15.244 0 0 0-.261-.263Z\"/></svg><h2 class=text>Wi-Fly</h2></div><form><textarea class=text id=fplan name=fplan placeholder=\"Paste your flightplan here...\"></textarea><br><br><input type=submit class=text id=upload value=Upload></form><p class=text>%s</p></body></html>"
// HTTP parameter that defines flightplan in GET request
#define FPLAN_PARAM "fplan="
#define FPLAN_PARAM_CONCAT "fplan=%s"

// Sizes of TCP buffers
#define TCP_HEADER_SIZE 1460 // Realistically, this should never need to go above 1460 because we can accumulate multiple headers
#define TCP_RESULT_SIZE 3660 // The result buffer size should be large enough to hold the entire HTTP response and page
// Keep in mind that if the content is made larger then both the result size and possibly LWIP's memory size need to be increased

typedef struct Waypoint {
    double lat;
    double lng;
    int alt;
} Waypoint;

#ifdef RASPBERRYPI_PICO_W

    /**
     * Initializes the Wi-Fly system.
    */
    void wifly_init();

    /**
     * De-initializes the Wi-Fly system.
    */
    void wifly_deinit();

#endif // RASPBERRYPI_PICO_W

/**
 * Generates the page content for Wi-Fly based on the current status of the flightplan.
 * @param result Pointer to the result buffer.
 * @param max_result_len The maximum length of the result buffer.
*/
int wifly_genPageContent(char *result, size_t max_result_len);

/**
 * Parses the Wi-Fly flightplan.
 * @param fplan The flightplan to parse.
 * @return true if the flightplan was successfully parsed, false if an error occured (more details in fplanStatus).
*/
bool wifly_parseFplan(const char *fplan);

/**
 * Gets the current flightplan (structured as a list of Waypoints).
 * @return A pointer to the current flightplan in Waypoint form, or NULL if there is none.
*/
Waypoint *wifly_getFplan();

/**
 * Gets the current flightplan (structured as JSON).
 * @return A pointer to the current flightplan in JSON form, or NULL if there is none.
*/
const char *wifly_getFplanJson();

/**
 * @return the number of waypoints in the current flightplan (0 if a flightplan has not yet been parsed).
*/
uint wifly_getWaypointCount();

#endif // __WIFLY_H
