/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <stdlib.h>

#include "platform/adc.h"
#include "platform/defs.h"

#include "io/aahrs.h"
#include "io/gps.h"

#include "modes/aircraft.h"

#include "sys/configuration.h"
#include "sys/print.h"

#include "get_sensor.h"

#if PLATFORM_SUPPORTS_ADC
// Print battery sensor data as JSON. No opening or closing braces are printed, just the data "key:value" pairs.
static inline void print_batt_data() {
    for (u32 i = 0; i < ADC_NUM_CHANNELS; i++) {
        if (i > 0)
            printraw(",");
        printraw("\"%lu\":%.6f", i, adc_read_raw(ADC_PINS[i]));
    }
}
#endif
// Print battery sensor data as JSON, including opening and closing braces. Should be printed as part of a larger JSON object.
static inline void print_batt() {
#if PLATFORM_SUPPORTS_ADC
    printraw(",\"batt\":{");
    print_batt_data();
    printraw("}");
#endif
}

i32 api_get_sensor(const char *args) {
    // Prepare the JSON output based on sensor type
    switch (atoi(args)) {
        case 1: // AAHRS only
            if (aircraft.aahrsSafe) {
                printraw("{\"aahrs\":{\"roll\":%.4f,\"pitch\":%.4f,\"yaw\":%.4f}}\n", aahrs.roll, aahrs.pitch, aahrs.yaw);
            } else {
                printraw("{\"aahrs\":{\"roll\":null,\"pitch\":null,\"yaw\":null}}\n");
            }
            break;
        case 2: // GPS only
            if (!gps.is_supported())
                return 403;
            if (aircraft.gpsSafe) {
                printraw("{\"gps\":{\"lat\":%.10Lf,\"lng\":%.10Lf,\"alt\":%ld,\"speed\":%.4f,\"track\":%.4f}}\n", gps.lat,
                         gps.lng, gps.alt, gps.speed, gps.track);
            } else {
                printraw("{\"gps\":{\"lat\":null,\"lng\":null,\"alt\":null,\"speed\":null,\"track\":null}}\n");
            }
            break;
#if PLATFORM_SUPPORTS_ADC
        case 3: // Battery only
            printraw("{\"batt\":{");
            print_batt_data();
            printraw("}}\n");
            break;
#endif
        case 0: // All sensors
        default:
            if (!gps.is_supported())
                return 403;
            if (aircraft.aahrsSafe && aircraft.gpsSafe) {
                printraw("{\"aahrs\":{\"roll\":%.4f,\"pitch\":%.4f,\"yaw\":%.4f},"
                         "\"gps\":{\"lat\":%.10Lf,\"lng\":%.10Lf,\"alt\":%ld,\"speed\":%.4f,\"track\":%.4f}",
                         aahrs.roll, aahrs.pitch, aahrs.yaw, gps.lat, gps.lng, gps.alt, gps.speed, gps.track);
                print_batt();
                printraw("}\n");
            } else if (aircraft.aahrsSafe) {
                printraw("{\"aahrs\":{\"roll\":%.4f,\"pitch\":%.4f,\"yaw\":%.4f},"
                         "\"gps\":{\"lat\":null,\"lng\":null,\"alt\":null,\"speed\":null,\"track\":null}",
                         aahrs.roll, aahrs.pitch, aahrs.yaw);
                print_batt();
                printraw("}\n");
            } else if (aircraft.gpsSafe) {
                printraw("{\"aahrs\":{\"roll\":null,\"pitch\":null,\"yaw\":null},"
                         "\"gps\":{\"lat\":%.10Lf,\"lng\":%.10Lf,\"alt\":%ld,\"speed\":%.4f,\"track\":%.4f}",
                         gps.lat, gps.lng, gps.alt, gps.speed, gps.track);
                print_batt();
                printraw("}\n");
            } else {
                printraw("{\"aahrs\":{\"roll\":null,\"pitch\":null,\"yaw\":null},"
                         "\"gps\":{\"lat\":null,\"lng\":null,\"alt\":null,\"speed\":null,\"track\":null}");
                print_batt();
                printraw("}\n");
            }
            break;
    }
    return -1;
}
