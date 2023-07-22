/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "../lib/minmea.h"

#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/uart.h"

#include "../modes/flight.h"
#include "../modes/modes.h"

#include "gps.h"

#ifdef GPS_ENABLED

/**
 * Reads a line from a specified UART instance if available.
 * @param uart The UART instance to read from.
 * @return A pointer to the read line, or NULL if no line could be read.
 * @note This function does not free the memory allocated for the line if read, ensure to free() it after use.
*/
static inline char *uart_read_line(uart_inst_t *uart) {
    char *buf = NULL;
    // If uart is readable, loop through characters and build string until we run out of characters or a newline
    if (uart_is_readable(uart)) {
        uint i = 0;
        while (uart_is_readable_within_us(uart, GPS_READ_TIMEOUT_US)) {
            char c = uart_getc(uart);
            if (c == '\r' || c == '\n') { // \r first optimized for NMEA
                break;
            }
            buf = realloc(buf, (i + 1) * sizeof(char));
            buf[i] = c;
            i++;
        }
        // Null terminate the string
        buf = realloc(buf, (i + 1) * sizeof(char));
        buf[i] = '\0';
    }
    return buf;
}

bool gps_init() {
    FBW_DEBUG_printf("[gps] initializing ");
    if (GPS_UART == uart0) {
        FBW_DEBUG_printf("uart0\n");
    } else if (GPS_UART == uart1) {
        FBW_DEBUG_printf("uart1\n");
    } else {
        FBW_DEBUG_printf("\n");
    }
    uart_init(GPS_UART, GPS_BAUDRATE);
    uart_set_fifo_enabled(GPS_UART, false); // Disable FIFO for setting up query schedule
    uart_set_format(GPS_UART, 8, 1, UART_PARITY_NONE); // NMEA-0183 format
    gpio_set_function(GPS_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(GPS_RX_PIN, GPIO_FUNC_UART);
    GPS_DEBUG_printf("[gps] setting up query schedule\n");
    // Clear FIFO and re-enable
    irq_set_enabled(GPS_UART_IRQ, true);
    uart_set_fifo_enabled(GPS_UART, true);
    // Send a command and wait until UART is ready to read, then read back the command response
    // Useful tool for calculating command checksums: https://nmeachecksum.eqth.net/
    #if defined(GPS_COMMAND_TYPE_PMTK)
        // PMTK manual: https://www.sparkfun.com/datasheets/GPS/Modules/PMTK_Protocol.pdf
        char *line0 = NULL;
        uart_write_blocking(GPS_UART, "$PMTK314,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0*29\r\n", 49); // VTG, GGA, GSA 1hz
        if (uart_is_readable_within_us(GPS_UART, GPS_COMMAND_TIMEOUT_US)) {
            line0 = uart_read_line(GPS_UART);
        } else {
            FBW_DEBUG_printf("[gps] ERROR: no response from GPS while setting query 0\n");
            return false;
        }
    #elif defined(GPS_COMMAND_TYPE_PSRF)
        // PSRF manual: https://www.sparkfun.com/datasheets/GPS/NMEA%20Reference%20Manual-Rev2.1-Dec07.pdf
        char *line0, *line1, *line2 = NULL;
        uart_write_blocking(GPS_UART, "$PSRF103,00,00,01,01*25\r\n", 27); // GGA, rate, 1hz, checksum enabled
        if (!uart_is_readable_within_us(GPS_UART, GPS_COMMAND_TIMEOUT_US)) {
            FBW_DEBUG_printf("[gps] ERROR: no response from GPS while setting query 0\n");
            return false;
        } else {
            line0 = uart_read_line(GPS_UART);
        }
        uart_write_blocking(GPS_UART, "$PSRF103,02,00,01,01*27\r\n", 27); // GSA, rate, 1hz, checksum enabled
        if (!uart_is_readable_within_us(GPS_UART, GPS_COMMAND_TIMEOUT_US)) {
            FBW_DEBUG_printf("[gps] ERROR: no response from GPS while setting query 1\n");
            return false;
        } else {
            line1 = uart_read_line(GPS_UART);
        }
        uart_write_blocking(GPS_UART, "$PSRF103,05,00,01,01*20\r\n", 27); // VTG, rate, 1hz, checksum enabled
        if (!uart_is_readable_within_us(GPS_UART, GPS_COMMAND_TIMEOUT_US)) {
            FBW_DEBUG_printf("[gps] ERROR: no response from GPS while setting query 2\n");
            return false;
        } else {
            line2 = uart_read_line(GPS_UART);
        }
    #else
        #error A GPS command type of PMTK or PSRF must be defined.
    #endif
    // Check for the correct response and return false if it doesn't match
    GPS_DEBUG_printf("[gps] validating query responses\n");
    #if defined(GPS_COMMAND_TYPE_PMTK)
        GPS_DEBUG_printf("[gps] response 0: %s\n", line0);
        bool result = (strncmp(line0, "$PMTK001,314,3*36", 46) == 0); // Acknowledged and successful execution of the command
        GPS_DEBUG_printf("[gps] comparison result: %d\n", strncmp(line0, "$PMTK001,314,3*36", 46));
        free(line0);
    #elif defined(GPS_COMMAND_TYPE_PSRF)
        GPS_DEBUG_printf("[gps] response 0: %s\n[gps] response 1: %s\n[gps] response 2: %s\n", line0, line1, line2);
        bool result = (strncmp(line0, "$PSRF103,00,00,01,01*25", 24) == 0 && strncmp(line1, "$PSRF103,02,00,01,01*27", 24) == 0 && strncmp(line2, "$PSRF103,05,00,01,01*20", 24) == 0);
        GPS_DEBUG_printf("[gps] comparison results: %d, %d, %d\n", strncmp(line0, "$PSRF103,00,00,01,01*25", 24), strncmp(line1, "$PSRF103,02,00,01,01*27", 24), strncmp(line2, "$PSRF103,05,00,01,01*20", 24));
        free(line0);
        free(line1);
        free(line2);
    #endif
    return result;
}

void gps_deinit() {
    uart_deinit(GPS_UART);
}

gpsData gps_getData() {
    /* These variables are static so that they persist between calls to gps_getData()
    This is due to the nature of how getting the data works--one line at a time
    If data was not saved between calls, one call would, for example, return correct coordinates but incorrect alt, and the next
    would return incorrect coordinates but correct alt. */
    static double lat, lng = -200.0;
    static int alt = -100;
    static float spd, trk = -100.0f;

    // Read a line from the GPS and parse it
    char *line = uart_read_line(GPS_UART);
    if (line != NULL) {
        switch (minmea_sentence_id(line, false)) {
            case MINMEA_SENTENCE_GGA: {
                minmea_sentence_gga gga;
                if (minmea_parse_gga(&gga, line)) {
                    lat = minmea_tocoord(&gga.latitude);
                    lng = minmea_tocoord(&gga.longitude);
                    if (strcmp(&gga.altitude_units, "M") == 0) {
                        alt = (int)(minmea_tofloat(&gga.altitude) * 3.28084f); // Conversion from meters to feet
                    } else {
                        FBW_DEBUG_printf("[gps] ERROR: unrecognized altitude unit");
                    }
                } else {
                    FBW_DEBUG_printf("[gps] ERROR: failed parsing $xxGGA sentence\n");
                }
                break;
            }
            case MINMEA_SENTENCE_GSA: {
                minmea_sentence_gsa gsa;
                if (minmea_parse_gsa(&gsa, line)) {
                    if (minmea_tofloat(&gsa.pdop) > GPS_SAFE_PDOP_THRESHOLD || minmea_tofloat(&gsa.hdop) > GPS_SAFE_HDOP_THRESHOLD || minmea_tofloat(&gsa.vdop) > GPS_SAFE_VDOP_THRESHOLD) {
                        // DOP value(s) too high
                        setGPSSafe(false);
                        return (gpsData){0};
                    }
                } else {
                    FBW_DEBUG_printf("[gps] ERROR: failed parsing $xxGSA sentence\n");
                }
                break;
            }
            case MINMEA_SENTENCE_VTG: {
                minmea_sentence_vtg vtg;
                if (minmea_parse_vtg(&vtg, line)) {
                    spd = minmea_tofloat(&vtg.speed_knots);
                    trk = minmea_tofloat(&vtg.magnetic_track_degrees);
                } else {
                    FBW_DEBUG_printf("[gps] ERROR: failed parsing $xxVTG sentence\n");
                }
                break;
            }
            
            case MINMEA_INVALID:
                /* This is TECHNICALLY an invalid sentence but minmea was flagging a bunch of sentences from before fixes are achieved
                as invalid so I just removed this debug statement because it was becoming very annoying and wasn't very useful anyway */

                // FBW_DEBUG_printf("[gps] ERROR: invalid sentence\n");
                break;
            case MINMEA_UNKNOWN:
                FBW_DEBUG_printf("[gps] WARNING: unsupported sentence\n");
            default:
                FBW_DEBUG_printf("[gps] ERROR: parse error\n");
                break;
        }
        free(line);
    }

    // Check for invalid data
    if (lat <= 90 && lat >= -90 && lng <= 180 && lng >= -180 && alt >= 0 && spd >= 0 && trk >= 0 && lat != INFINITY && lng != INFINITY && alt != INFINITY && spd != INFINITY && trk != INFINITY && lat != NAN && lng != NAN && alt != NAN && spd != NAN && trk != NAN) {
        setGPSSafe(true);
    } else {
        setGPSSafe(false);
        return (gpsData){0};
    }
    return (gpsData){lat, lng, alt, spd, trk};
}

#endif // GPS_ENABLED
