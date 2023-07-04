/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdint.h>

#include "hardware/i2c.h"

#include "gps.h"

int gps_init() {
    #ifdef GPS_ADAFRUIT_ULT
        // TODO: gps things (once I actually get one...)
    #else
        #error No GPS module was defined.
    #endif
}

void gps_deinit() {
    
}

gpsData gps_getData() {
    // TODO; more gps things here
    // I just had to create this function so I can integrate it into auto mode now
    double lat = 0;
    double lng = 0;
    int_fast16_t alt = 0;
    float speed = 0;
    return (gpsData){lat, lng, alt, speed};
}
