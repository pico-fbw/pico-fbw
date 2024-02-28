/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
*/

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "platform/time.h"
#include "platform/uart.h"

#include "lib/minmea.h"

#include "modes/flight.h"
#include "modes/aircraft.h"

#include "sys/configuration.h"
#include "sys/log.h"
#include "sys/print.h"

#include "gps.h"

// These are the DOP thresholds to accept for safe flying, if any of the DOPs are larger than this the GPS will be considered unsafe
#define GPS_SAFE_PDOP_THRESHOLD 4
#define GPS_SAFE_HDOP_THRESHOLD 5
#define GPS_SAFE_VDOP_THRESHOLD 3

#define M_TO_FT 3.28084f // Meters to feet conversion constant

static inline bool pos_valid(float lat, float lng) {
    return lat <= 90 && lat >= -90 && lng <= 180 && lng >= -180 &&
           isfinite(lat) && isfinite(lng);
}

static inline bool alt_valid(i32 alt) {
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

static inline bool data_valid(float lat, float lng, i32 alt, float speed, float track, float pdop, float hdop, float vdop) {
    return pos_valid(lat, lng) && alt_valid(alt) && speed_valid(speed) && track_valid(track) && dop_valid(pdop, hdop, vdop);
}

bool gps_init() {
    printfbw(gps, "initializing uart at baudrate %d, on pins %d (tx) and %d (rx)", (u32)config.sensors[SENSORS_GPS_BAUDRATE],
                          (u32)config.pins[PINS_GPS_TX], (u32)config.pins[PINS_GPS_RX]);
    uart_setup((u32)config.pins[PINS_GPS_TX], (u32)config.pins[PINS_GPS_RX], (u32)config.sensors[SENSORS_GPS_BAUDRATE]);
    printfbw(gps, "configuring...");
    // Send a command and wait until UART is ready to read, then read back the command response
    // Useful tool for calculating command checksums: https://nmeachecksum.eqth.net/
    printfbw(gps, "setting up query schedule");
    switch ((GPSCommandType)config.sensors[SENSORS_GPS_COMMAND_TYPE]) {
        case GPS_COMMAND_TYPE_PMTK:
            // PMTK manual: https://cdn.sparkfun.com/assets/parts/1/2/2/8/0/PMTK_Packet_User_Manual.pdf
            // Enable the correct sentences
            sleep_ms_blocking(1800); // Acknowledgement is a hit or miss without a delay
            // VTG enabled 5x per fix (for fast track updates), GGA, GSA enabled once per fix
            uart_write((u32)config.pins[PINS_GPS_TX], (u32)config.pins[PINS_GPS_RX], "$PMTK314,0,0,5,1,1,0,0,0,0,0,0,0,0,0,0,0,0*2D\r\n");
            // Check up to 30 sentences or up to 3 seconds for the acknowledgement
            u8 lines = 0;
            Timestamp timeout = timestamp_in_ms(3000);
            while (lines < 30 && !timestamp_reached(&timeout)) {
                char *line = uart_read((u32)config.pins[PINS_GPS_TX], (u32)config.pins[PINS_GPS_RX]);
                printfbw(gps, "response %d: %s", lines, line);
                bool result = (strncmp(line, "$PMTK001,314,3*36", 17) == 0); // Acknowledged and successful execution of the command
                free(line);
                if (result) return true;
                lines++;
            }
            if (timestamp_reached(&timeout)) printfbw(gps, "ERROR: communication with GPS timed out!");
            printfbw(gps, "ERROR: %d responses were checked but none were valid!", lines);
            return false;
            break;
        default:
            return false;
    }
}

void gps_update() {
    // Read line(s) from the GPS and parse them until there are none remaining
    char *line = uart_read((u32)config.pins[PINS_GPS_TX], (u32)config.pins[PINS_GPS_RX]);
    while (line) {
        switch (minmea_sentence_id(line, false)) {
            case MINMEA_SENTENCE_GGA: {
                struct minmea_sentence_gga gga;
                if (minmea_parse_gga(&gga, line)) {
                    gps.lat = minmea_tocoord(&gga.latitude);
                    gps.lng = minmea_tocoord(&gga.longitude);
                    if (strncmp(&gga.altitude_units, "M", 1) == 0) {
                        gps.alt = (i32)(minmea_tofloat(&gga.altitude) * M_TO_FT);
                    } else {
                        aircraft.setGPSSafe(false);
                        printfbw(gps, "ERROR: incorrect altitude units!");
                        return;
                    }
                } else {
                    printfbw(gps, "ERROR: failed parsing $xxGGA sentence");
                }
                break;
            }
            case MINMEA_SENTENCE_GSA: {
                struct minmea_sentence_gsa gsa;
                if (minmea_parse_gsa(&gsa, line)) {
                    gps.pdop = minmea_tofloat(&gsa.pdop);
                    gps.hdop = minmea_tofloat(&gsa.hdop);
                    gps.vdop = minmea_tofloat(&gsa.vdop);
                } else {
                    printfbw(gps, "ERROR: failed parsing $xxGSA sentence");
                }
                break;
            }
            case MINMEA_SENTENCE_VTG: {
                struct minmea_sentence_vtg vtg;
                if (minmea_parse_vtg(&vtg, line)) {
                    gps.speed = minmea_tofloat(&vtg.speed_knots);
                    gps.track = minmea_tofloat(&vtg.true_track_degrees);
                } else {
                    printfbw(gps, "ERROR: failed parsing $xxVTG sentence");
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
        line = uart_read((u32)config.pins[PINS_GPS_TX], (u32)config.pins[PINS_GPS_RX]);
    }
    aircraft.setGPSSafe(data_valid(gps.lat, gps.lng, gps.alt, gps.speed, gps.track, gps.pdop, gps.hdop, gps.vdop));
}

i32 gps_calibrate_alt_offset(u32 num_samples) {
    log_message(INFO, "Calibrating altitude", 1000, 100, false);
    // GPS updates should be at 1Hz (give or take 2s) so if the calibration takes longer we cut it short
    Timestamp calibrationTimeout = timestamp_in_ms((num_samples * 1000) + 2000);
    u32 samples = 0;
    i64 alts = 0;
    while (samples < num_samples && !timestamp_reached(&calibrationTimeout)) {
        char *line = uart_read((u32)config.pins[PINS_GPS_TX], (u32)config.pins[PINS_GPS_RX]);
        if (line) {
            switch (minmea_sentence_id(line, false)) {
                case MINMEA_SENTENCE_GGA: {
                    struct minmea_sentence_gga gga;
                    if (minmea_parse_gga(&gga, line)) {
                        if (strncmp(&gga.altitude_units, "M", 1) == 0) {
                            i32 calt = (i32)(minmea_tofloat(&gga.altitude) * 3.28084f);
                            alts += calt;
                            printfbw(gps, "altitude: %d (%d of %d)", calt, samples + 1, num_samples);
                        } else {
                            printfbw(gps, "ERROR: invalid altitude units during calibration");
                            return -2;
                        }
                    } else {
                        printfbw(gps, "ERROR: failed parsing $xxGGA sentence during calibration");
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
    if (timestamp_reached(&calibrationTimeout)) {
        printfbw(gps, "ERROR: altitude calibration timed out");
        return -1;
    } else {
        gps.altOffset = (i32)(alts / samples);
        printfbw(gps, "altitude offset calculated as: %d", gps.altOffset);
        gps.altOffsetCalibrated = true;
        return 0;
    }
}

bool gps_is_supported() { return ((GPSCommandType)config.sensors[SENSORS_GPS_COMMAND_TYPE] != GPS_COMMAND_TYPE_NONE); }

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
    .altOffsetCalibrated = false,
    .init = gps_init,
    .update = gps_update,
    .calibrate_alt_offset = gps_calibrate_alt_offset,
    .is_supported = gps_is_supported
};
