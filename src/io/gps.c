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