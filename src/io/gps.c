/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "pico/time.h"
#include "pico/types.h"

#include "hardware/gpio.h"
#include "hardware/uart.h"

#include "../lib/minmea.h"

#include "../modes/flight.h"
#include "../modes/modes.h"

#include "../sys/config.h"
#include "../sys/log.h"

#include "serial.h"

#include "gps.h"

bool gps_init() {
    if (config.debug.debug_fbw) printf("[gps] initializing ");
    if (GPS_UART == uart0) {
        if (config.debug.debug_fbw) printf("uart0\n");
    } else if (GPS_UART == uart1) {
        if (config.debug.debug_fbw) printf("uart1\n");
    } else {
        if (config.debug.debug_fbw) printf("\n");
    }
    uart_init(GPS_UART, config.sensors.gpsBaudrate);
    uart_set_fifo_enabled(GPS_UART, false); // Disable FIFO for setting up query schedule
    uart_set_format(GPS_UART, 8, 1, UART_PARITY_NONE); // NMEA-0183 format
    gpio_set_function(config.sensors.gpsTx, GPIO_FUNC_UART);
    gpio_set_function(config.sensors.gpsRx, GPIO_FUNC_UART);
    gpio_pull_up(config.sensors.gpsTx);
    gpio_pull_up(config.sensors.gpsRx);
    if (config.debug.debug_fbw) printf("[gps] configuring...\n");
    // Clear FIFO and re-enable
    irq_set_enabled(GPS_UART_IRQ, true);
    uart_set_fifo_enabled(GPS_UART, true);
    // Send a command and wait until UART is ready to read, then read back the command response
    // Useful tool for calculating command checksums: https://nmeachecksum.eqth.net/
    if (config.debug.debug_gps) printf("[gps] setting up query schedule\n");
    switch (config.sensors.gpsCommandType) {
        case GPS_COMMAND_TYPE_PMTK:
            // PMTK manual: https://cdn.sparkfun.com/assets/parts/1/2/2/8/0/PMTK_Packet_User_Manual.pdf
            // Enable the correct sentences
            sleep_ms(1800); // Acknowledgement is a hit or miss without a delay
            uart_write_blocking(GPS_UART, "$PMTK314,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0*29\r\n", 49); // VTG, GGA, GSA enabled once per fix
            // Check up to 20 sentences for the acknowledgement
            uint8_t lines = 0;
            while (lines < 20) {
                char *line = NULL;
                if (uart_is_readable_within_us(GPS_UART, GPS_COMMAND_TIMEOUT_MS * 1000)) {
                    line = uart_read_line(GPS_UART);
                    if (config.debug.debug_gps) printf("[gps] response %d: %s\n", lines, line);
                    bool result = (strncmp(line, "$PMTK001,314,3*36", 17) == 0); // Acknowledged and successful execution of the command
                    free(line);
                    if (result) return true;
                    lines++;
                } else {
                    if (config.debug.debug_fbw) printf("[gps] ERROR: timed out whilst awaiting a response!\n");
                    return false;
                }
            }
            if (config.debug.debug_fbw) printf("[gps] ERROR: %d responses were checked but none were valid!\n", lines);
            return false;
            break;
        default:
            return false;
    }
}

void gps_deinit() { uart_deinit(GPS_UART); }

GPS gps_getData() {
    /* These variables are static so that they persist between calls to gps_getData()
    This is due to the nature of how getting the data works--one line at a time
    If data was not saved between calls, one call would, for example, return correct coordinates but incorrect alt, and the next
    would return incorrect coordinates but correct alt. */
    static long double lat, lng = -200.0;
    static int alt = -100;
    static float spd, trk_true = -100.0f;
    static float pdop, hdop, vdop = -1.0f;

    // Read a line from the GPS and parse it
    char *line = uart_read_line(GPS_UART);
    if (line != NULL) {
        switch (minmea_sentence_id(line, false)) {
            case MINMEA_SENTENCE_GGA: {
                minmea_sentence_gga gga;
                if (minmea_parse_gga(&gga, line)) {
                    lat = minmea_tocoord(&gga.latitude);
                    lng = minmea_tocoord(&gga.longitude);
                    if (strncmp(&gga.altitude_units, "M", 1) == 0) {
                        alt = (int)(minmea_tofloat(&gga.altitude) * 3.28084f); // Conversion from meters to feet
                    } else {
                        setGPSSafe(false);
                        return (GPS){INFINITY};
                    }
                } else {
                    if (config.debug.debug_gps) printf("[gps] ERROR: failed parsing $xxGGA sentence\n");
                }
                break;
            }
            case MINMEA_SENTENCE_GSA: {
                minmea_sentence_gsa gsa;
                if (minmea_parse_gsa(&gsa, line)) {
                    pdop = minmea_tofloat(&gsa.pdop);
                    hdop = minmea_tofloat(&gsa.hdop);
                    vdop = minmea_tofloat(&gsa.vdop);
                } else {
                    if (config.debug.debug_gps) printf("[gps] ERROR: failed parsing $xxGSA sentence\n");
                }
                break;
            }
            case MINMEA_SENTENCE_VTG: {
                minmea_sentence_vtg vtg;
                if (minmea_parse_vtg(&vtg, line)) {
                    spd = minmea_tofloat(&vtg.speed_knots);
                    trk_true = minmea_tofloat(&vtg.true_track_degrees);
                } else {
                    if (config.debug.debug_gps) printf("[gps] ERROR: failed parsing $xxVTG sentence\n");
                }
                break;
            }
            
            /* All of these indicate parse errors but happen every so often and don't really mean anything, so they do not warrant a message. */
            case MINMEA_INVALID:
            case MINMEA_UNKNOWN:
            default:
                break;
        }
        free(line);
    }

    if (lat <= 90 && lat >= -90 && lng <= 180 && lng >= -180 && alt >= 0 && spd >= 0 && lat != INFINITY && lng != INFINITY && alt != INFINITY && spd != INFINITY && trk_true != INFINITY && lat != NAN && lng != NAN && alt != NAN && spd != NAN && trk_true != NAN && pdop < GPS_SAFE_PDOP_THRESHOLD && hdop < GPS_SAFE_HDOP_THRESHOLD && vdop < GPS_SAFE_VDOP_THRESHOLD) {
        setGPSSafe(true);
    } else {
        setGPSSafe(false);
        return (GPS){INFINITY};
    }
    return (GPS){lat, lng, alt, spd, trk_true};
}

static int altOffset = 0; // The offset of the aircraft's altitude in feet
int gps_getAltOffset() { return altOffset; }

static bool altOffsetCalibrated = false;
bool gps_isAltOffsetCalibrated() { return altOffsetCalibrated; }

int gps_calibrateAltOffset(uint num_samples) {
    log_message(INFO, "Calibrating altitude", 1000, 100, false);
    // GPS updates should be at 1Hz (give or take 2s) so if the calibration takes longer we cut it short
    absolute_time_t calibrationTimeout = make_timeout_time_ms((num_samples * 1000) + 2000);
    uint samples = 0;
    int64_t alts = 0;
    while (samples < num_samples && !time_reached(calibrationTimeout)) {
        // printf("time reached: %d\n", time_reached(calibrationTimeout));
        char *line = uart_read_line(GPS_UART);
        if (line != NULL) {
            switch (minmea_sentence_id(line, false)) {
                case MINMEA_SENTENCE_GGA: {
                    minmea_sentence_gga gga;
                    if (minmea_parse_gga(&gga, line)) {
                        if (strncmp(&gga.altitude_units, "M", 1) == 0) {
                            int calt = (int)(minmea_tofloat(&gga.altitude) * 3.28084f);
                            alts += calt;
                            if (config.debug.debug_gps) printf("[gps] altitude: %d (%d of %d)\n", calt, samples + 1, num_samples);
                        } else {
                            if (config.debug.debug_fbw) printf("[gps] ERROR: invalid altitude units during calibration\n");
                            return PICO_ERROR_GENERIC;
                        }
                    } else {
                        if (config.debug.debug_gps) printf("[gps] ERROR: failed parsing $xxGGA sentence during calibration\n");
                    }
                    samples++;
                }
            }
            free(line);
        }
    }
    log_clear(INFO);
    if (time_reached(calibrationTimeout)) {
        if (config.debug.debug_fbw) printf("[gps] ERROR: altitude calibration timed out\n");
        return PICO_ERROR_TIMEOUT;
    } else {
        altOffset = alts / samples;
        if (config.debug.debug_fbw) printf("[gps] altitude offset calculated as: %d\n", altOffset);
        altOffsetCalibrated = true;
        return 0;
    }
}
