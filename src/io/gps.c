/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>
#include <stdio.h>
#include "pico/binary_info.h"

#include "hardware/gpio.h"
#include "hardware/uart.h"

#include "../modes/modes.h"

#include "gps.h"

// currently porting from https://github.com/CAProjects/Adafruit-GPS-Pico/blob/main/GPS_Pico.py until my gps arrives
// other links:
// https://files.banggood.com/2016/11/BN-220%20GPS+Antenna%20datasheet.pdf
// https://en.wikipedia.org/wiki/NMEA_0183
// https://github.com/kosma/minmea

bool gps_init() {
    FBW_DEBUG_printf("[gps] initializing ");
    if (GPS_UART == uart0) {
        FBW_DEBUG_printf("uart0\n");
    } else if (GPS_UART == uart1) {
        FBW_DEBUG_printf("uart1\n");
    } else {
        FBW_DEBUG_printf("\n");
    }
    uart_init(GPS_UART, GPS_BAUD_RATE);
    uart_set_format(GPS_UART, 8, 1, UART_PARITY_NONE);
    gpio_set_function(GPS_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(GPS_RX_PIN, GPIO_FUNC_UART);
    bi_decl(bi_2pins_with_func(GPS_TX_PIN, GPS_RX_PIN, GPIO_FUNC_UART));
    // TODO: test commms with gps and return true/false
    return true;
}

void gps_deinit() {
    uart_deinit(GPS_UART);
}

gpsData gps_getData() {
    // TODO: read and parse data NMEA-0183

    double lat = 0;
    double lng = 0;
    int alt = 0;
    float spd = 0;
    return (gpsData){lat, lng, alt, spd};
}
