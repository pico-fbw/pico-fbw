/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "pico/time.h"

#include "hardware/gpio.h"
#include "hardware/uart.h"

#include "../lib/minmea.h"

#include "../modes/flight.h"
#include "../modes/modes.h"

#include "../sys/log.h"

#include "flash.h"
#include "serial.h"

#include "gps.h"

static inline bool pos_valid(float lat, float lng) {
    return lat <= 90 && lat >= -90 && lng <= 180 && lng >= -180 &&
           isfinite(lat) && isfinite(lng);
}

static inline bool alt_valid(int alt) {
    return alt >= 0;
}

static inline bool speed_valid(float speed) {
    return speed >= 0 && isfinite(speed);
}

static inline bool track_valid(float track) {
    return track >= 0 && isfinite(track);
}

// (DOP stands for dilution of precision, basically a mesaure of how confident the GPS is in its output)
static inline bool dop_valid(float pdop, float hdop, float vdop) {
    return pdop < GPS_SAFE_PDOP_THRESHOLD && hdop < GPS_SAFE_HDOP_THRESHOLD &&
           vdop < GPS_SAFE_VDOP_THRESHOLD;
}

static inline bool data_valid(float lat, float lng, int alt, float speed, float track, float pdop, float hdop, float vdop) {
    return pos_valid(lat, lng) && alt_valid(alt) && speed_valid(speed) && track_valid(track) && dop_valid(pdop, hdop, vdop);
}

bool gps_init() {
    if (print.fbw) printf("[gps] initializing ");
    if (GPS_UART == uart0) {
        if (print.fbw) printf("uart0 ");
    } else if (GPS_UART == uart1) {
        if (print.fbw) printf("uart1 ");
    }
    if (print.fbw) printf("at baudrate %d, on pins %d (tx) and %d (rx)\n", (uint)flash.sensors[SENSORS_GPS_BAUDRATE],
                          (uint)flash.pins[PINS_GPS_TX], (uint)flash.pins[PINS_GPS_RX]);
    uart_init(GPS_UART, (uint)flash.sensors[SENSORS_GPS_BAUDRATE]);
    uart_set_fifo_enabled(GPS_UART, false); // Disable FIFO for setting up query schedule
    uart_set_format(GPS_UART, 8, 1, UART_PARITY_NONE); // NMEA-0183 format
    gpio_set_function((uint)flash.pins[PINS_GPS_TX], GPIO_FUNC_UART);
    gpio_set_function((uint)flash.pins[PINS_GPS_RX], GPIO_FUNC_UART);
    gpio_pull_up((uint)flash.pins[PINS_GPS_TX]);
    gpio_pull_up((uint)flash.pins[PINS_GPS_RX]);
    if (print.fbw) printf("[gps] configuring...\n");
    // Clear FIFO and re-enable
    irq_set_enabled(GPS_UART_IRQ, true);
    uart_set_fifo_enabled(GPS_UART, true);
    // Send a command and wait until UART is ready to read, then read back the command response
    // Useful tool for calculating command checksums: https://nmeachecksum.eqth.net/
    if (print.gps) printf("[gps] setting up query schedule\n");
    switch ((GPSCommandType)flash.sensors[SENSORS_GPS_COMMAND_TYPE]) {
        case GPS_COMMAND_TYPE_PMTK:
            // PMTK manual: https://cdn.sparkfun.com/assets/parts/1/2/2/8/0/PMTK_Packet_User_Manual.pdf
            // Enable the correct sentences
            sleep_ms(1800); // Acknowledgement is a hit or miss without a delay
            // VTG enabled 5x per fix (for fast track updates), GGA, GSA enabled once per fix
            uart_write_blocking(GPS_UART, (const uint8_t*)"$PMTK314,0,0,5,1,1,0,0,0,0,0,0,0,0,0,0,0,0*2D\r\n", 49);
            // Check up to 30 sentences for the acknowledgement
            uint8_t lines = 0;
            while (lines < 30) {
                char *line = NULL;
                if (uart_is_readable_within_us(GPS_UART, GPS_COMMAND_TIMEOUT_MS * 1000)) {
                    line = uart_read_line(GPS_UART);
                    if (print.gps) printf("[gps] response %d: %s\n", lines, line);
                    bool result = (strncmp(line, "$PMTK001,314,3*36", 17) == 0); // Acknowledged and successful execution of the command
                    free(line);
                    if (result) return true;
                    lines++;
                } else {
                    if (print.fbw) printf("[gps] ERROR: timed out whilst awaiting a response!\n");
                    return false;
                }
            }
            if (print.fbw) printf("[gps] ERROR: %d responses were checked but none were valid!\n", lines);
            return false;
            break;
        default:
            return false;
    }
}

void gps_deinit() { uart_deinit(GPS_UART); }

void gps_update() {
    // Read line(s) from the GPS and parse them until there are none remaining
    char *line = uart_read_line(GPS_UART);
    while (line != NULL) {
        switch (minmea_sentence_id(line, false)) {
            case MINMEA_SENTENCE_GGA: {
                minmea_sentence_gga gga;
                if (minmea_parse_gga(&gga, line)) {
                    gps.lat = minmea_tocoord(&gga.latitude);
                    gps.lng = minmea_tocoord(&gga.longitude);
                    if (strncmp(&gga.altitude_units, "M", 1) == 0) {
                        gps.alt = (int)(minmea_tofloat(&gga.altitude) * M_TO_FT);
                    } else {
                        aircraft.setGPSSafe(false);
                        if (print.fbw) printf("[gps] ERROR: incorrect altitude units!\n");
                        return;
                    }
                } else {
                    if (print.gps) printf("[gps] ERROR: failed parsing $xxGGA sentence\n");
                }
                break;
            }
            case MINMEA_SENTENCE_GSA: {
                minmea_sentence_gsa gsa;
                if (minmea_parse_gsa(&gsa, line)) {
                    gps.pdop = minmea_tofloat(&gsa.pdop);
                    gps.hdop = minmea_tofloat(&gsa.hdop);
                    gps.vdop = minmea_tofloat(&gsa.vdop);
                } else {
                    if (print.gps) printf("[gps] ERROR: failed parsing $xxGSA sentence\n");
                }
                break;
            }
            case MINMEA_SENTENCE_VTG: {
                minmea_sentence_vtg vtg;
                if (minmea_parse_vtg(&vtg, line)) {
                    gps.speed = minmea_tofloat(&vtg.speed_knots);
                    gps.track = minmea_tofloat(&vtg.true_track_degrees);
                } else {
                    if (print.gps) printf("[gps] ERROR: failed parsing $xxVTG sentence\n");
                }
                break;
            }
            
            // All of these indicate parse errors but happen every so often and don't really mean anything, so they do not warrant a message
            case MINMEA_INVALID:
            case MINMEA_UNKNOWN:
            default:
                break;
        }
        // Clear the line and attempt to read in a new one
        free(line);
        line = uart_read_line(GPS_UART);
    }
    aircraft.setGPSSafe(data_valid(gps.lat, gps.lng, gps.alt, gps.speed, gps.track, gps.pdop, gps.hdop, gps.vdop));
}

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
                            if (print.gps) printf("[gps] altitude: %d (%d of %d)\n", calt, samples + 1, num_samples);
                        } else {
                            if (print.fbw) printf("[gps] ERROR: invalid altitude units during calibration\n");
                            return PICO_ERROR_GENERIC;
                        }
                    } else {
                        if (print.gps) printf("[gps] ERROR: failed parsing $xxGGA sentence during calibration\n");
                    }
                    samples++;
                }
                default: {
                    break;
                }
            }
            free(line);
        }
    }
    log_clear(INFO);
    if (time_reached(calibrationTimeout)) {
        if (print.fbw) printf("[gps] ERROR: altitude calibration timed out\n");
        return PICO_ERROR_TIMEOUT;
    } else {
        gps.altOffset = (int)(alts / samples);
        if (print.fbw) printf("[gps] altitude offset calculated as: %d\n", gps.altOffset);
        gps.altOffset_calibrated = true;
        return 0;
    }
}

bool gps_isSupported() { return ((GPSCommandType)flash.sensors[SENSORS_GPS_COMMAND_TYPE] != GPS_COMMAND_TYPE_NONE); }

GPS gps = {
    .lat = -200.0,
    .lng = -200.0,
    .alt = -1,
    .speed = -1.0f,
    .track = -1.0f,
    .pdop = -1.0f,
    .hdop = -1.0f,
    .vdop = -1.0f,
    .altOffset = 0,
    .altOffset_calibrated = false,
    .init = gps_init,
    .deinit = gps_deinit,
    .update = gps_update,
    .calibrateAltOffset = gps_calibrateAltOffset,
    .isSupported = gps_isSupported
};
