/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>
#if SIMCONNECT
    #include "platform/simconnect.h"
#endif
#include "platform/time.h"
#include "platform/uart.h"

#include "ctrl/aircraft.h"
#include "lib/minmea.h"
#include "sys/configuration.h"
#include "sys/log.h"
#include "sys/print.h"
#include "sys/runtime.h"

#include "gps.h"

#define TX (i16) config.pins.gpsTx
#define RX (i16) config.pins.gpsRx
// If any DOPs are larger than this the GPS will be considered unsafe
#define GPS_SAFE_PDOP_THRESHOLD 4
#define GPS_SAFE_HDOP_THRESHOLD 5
#define GPS_SAFE_VDOP_THRESHOLD 3
// Maximum number of acknowledgement lines to check
#define MAX_ACK_LINES 30
// Timeout for acknowledgement in milliseconds
#define ACK_TIMEOUT_MS 3000

#define M_TO_FT 3.28084f // Meters to feet conversion constant

static inline bool pos_valid(f32 lat, f32 lng) {
    return lat <= 90 && lat >= -90 && lng <= 180 && lng >= -180 && isfinite(lat) && isfinite(lng);
}

static inline bool alt_valid(i32 alt) {
    return alt >= 0;
}

static inline bool speed_valid(f32 speed) {
    return speed >= 0 && isfinite(speed);
}

static inline bool track_valid(f32 track) {
    return track >= 0 && isfinite(track);
}

// (DOP stands for dilution of precision, basically a mesaure of how confident the GPS is in its output)
static inline bool dop_valid(f32 pdop, f32 hdop, f32 vdop) {
    return pdop < GPS_SAFE_PDOP_THRESHOLD && hdop < GPS_SAFE_HDOP_THRESHOLD && vdop < GPS_SAFE_VDOP_THRESHOLD;
}

static inline bool data_valid(f32 lat, f32 lng, i32 alt, f32 speed, f32 track, f32 pdop, f32 hdop, f32 vdop) {
    return pos_valid(lat, lng) && alt_valid(alt) && speed_valid(speed) && track_valid(track) &&
           dop_valid(pdop, hdop, vdop);
}

/**
 * Waits for and validates acknowledgement from a GPS command.
 * @param ack acknowledgement to look for
 * @return true if valid acknowledgement received
 */
static bool wait_for_ack(const char *ack) {
    u8 lines = 0;
    Timestamp timeout = timestamp_in_ms(ACK_TIMEOUT_MS);
    while (lines < MAX_ACK_LINES && !timestamp_reached(&timeout)) {
        char *line = uart_read(TX, RX);
        if (!line) {
            continue;
        }
        printsys(gps, "response %d: %s", lines, line);
        bool result = (strncmp(line, ack, strlen(ack)) == 0);
        free(line);
        if (result) {
            return true;
        }
        lines++;
    }
    if (timestamp_reached(&timeout)) {
        printsys(gps, "ERROR: communication with GPS timed out!");
    } else {
        printsys(gps, "ERROR: %d responses were checked but none were valid!", lines);
    }
    return false;
}

// Initializes a module using the PMTK command set.
static bool init_pmtk() {
    // PMTK manual: https://cdn.sparkfun.com/assets/parts/1/2/2/8/0/PMTK_Packet_User_Manual.pdf
    printsys(gps, "setting query schedule");
    sleep_ms_blocking(1800); // Acknowledgement is a hit or miss without a delay
    // VTG enabled 5x per fix (for fast track updates), GGA, GSA enabled once per fix
    uart_write(TX, RX, "$PMTK314,0,0,5,1,1,0,0,0,0,0,0,0,0,0,0,0,0*2D\r\n");
    // Check up to 30 sentences or up to 3 seconds for the acknowledgement
    return wait_for_ack("$PMTK001,314,3*36");
}

// Initializes a module using the PQMT command set.
static bool init_pqmt() {
    // PQMT command set: https://github.com/sparkfun/SparkFun_GNSS_LG580P/blob/main/docs/pqmt_commands.md
    printsys(gps, "setting fix interval");
    uart_write(TX, RX, "$PQTMCFGFIXRATE,W,200*6A\r\n"); // 5Hz fixes
    return wait_for_ack("$PQTMCFGFIXRATE,OK*27");
}

// Initializes a module using the Quectel LC260Z/LC76xZ proprietary command set.
static bool init_quectel_lc_26_76() {
    // Set 200ms (5Hz) fix interval
    uart_write(TX, RX, (char[]){0xF1, 0xD9, 0x06, 0x42, 0x14, 0x00, 0x00, 0x01, 0x35, 0x32, 0xC8, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8C, 0xD5});
    // Response should be F1 D9 05 01 02 00 06 42 50 79
    // Enable VTG once per fix
    uart_write(TX, RX, "$PQTMCFGMSGRATE,W,VTG,1*0E\r\n");
    if (!wait_for_ack("$PQTMCFGMSGRATE,OK*29")) {
        return false;
    }
    // Enable GGA and GSA once every 5 fixes
    uart_write(TX, RX, "$PQTMCFGMSGRATE,W,GGA,5*0E\r\n");
    if (!wait_for_ack("$PQTMCFGMSGRATE,OK*29")) {
        return false;
    }
    uart_write(TX, RX, "$PQTMCFGMSGRATE,W,GSA,5*1A\r\n");
    return wait_for_ack("$PQTMCFGMSGRATE,OK*29");
}

/**
 * Parses GGA sentence and updates GPS position data.
 * @param line NMEA sentence line to parse
 * @return true if parsing was successful
 */
static bool parse_gga_sentence(const char *line) {
    struct minmea_sentence_gga gga;
    if (!minmea_parse_gga(&gga, line)) {
        printsys(gps, "ERROR: failed parsing $xxGGA sentence: %s", line);
        return false;
    }
    gps.lat = minmea_tocoord(&gga.latitude);
    gps.lng = minmea_tocoord(&gga.longitude);

    if (strncmp(&gga.altitude_units, "M", 1) != 0) {
        aircraft_set_gps_safe(false);
        printsys(gps, "ERROR: incorrect altitude units!");
        return false;
    }
    gps.alt = (i32)(minmea_tofloat(&gga.altitude) * M_TO_FT);
    gps.sats = gga.satellites_tracked;
    return true;
}

/**
 * Parses GSA sentence and updates GPS DOP data.
 * @param line NMEA sentence line to parse
 */
static void parse_gsa_sentence(const char *line) {
    struct minmea_sentence_gsa gsa;
    if (minmea_parse_gsa(&gsa, line)) {
        gps.pdop = minmea_tofloat(&gsa.pdop);
        gps.hdop = minmea_tofloat(&gsa.hdop);
        gps.vdop = minmea_tofloat(&gsa.vdop);
    } else {
        printsys(gps, "ERROR: failed parsing $xxGSA sentence: %s", line);
    }
}

/**
 * Parses VTG sentence and updates GPS speed/track data.
 * @param line NMEA sentence line to parse
 */
static void parse_vtg_sentence(const char *line) {
    struct minmea_sentence_vtg vtg;
    if (minmea_parse_vtg(&vtg, line)) {
        gps.speed = minmea_tofloat(&vtg.speed_knots);
        gps.track = minmea_tofloat(&vtg.true_track_degrees);
    } else {
        printsys(gps, "ERROR: failed parsing $xxVTG sentence: %s", line);
    }
}

/**
 * Processes a single NMEA sentence from the GPS.
 * @param line NMEA sentence line to process
 * @return true if processing should continue, false if it should stop
 */
static bool process_nmea_sentence(const char *line) {
    switch (minmea_sentence_id(line, false)) {
        case MINMEA_SENTENCE_GGA:
            return parse_gga_sentence(line);
        case MINMEA_SENTENCE_GSA:
            parse_gsa_sentence(line);
            break;
        case MINMEA_SENTENCE_VTG:
            parse_vtg_sentence(line);
            break;
        // All of these indicate parse errors but happen every so often and don't really mean anything,
        // so they do not warrant a message
        case MINMEA_INVALID:
        case MINMEA_UNKNOWN:
        default:
            break;
    }
    return true;
}

bool gps_init() {
#if !SIMCONNECT
    printsys(gps, "initializing uart at baudrate %lu, on pins %d (tx) and %d (rx)", (u32)config.sensors.gpsBaudrate, TX,
             RX);
    uart_setup(TX, RX, (u32)config.sensors.gpsBaudrate);
    printsys(gps, "configuring...");

    // Send a command and wait until UART is ready to read, then read back the command response
    // Useful tool for calculating command checksums: https://nmeachecksum.eqth.net/
    switch ((GPSCommandType)config.sensors.gpsCommandType) {
        case GPS_COMMAND_TYPE_PMTK:
            return init_pmtk();
        case GPS_COMMAND_TYPE_PQMT:
            return init_pqmt();
        case GPS_COMMAND_TYPE_QUECTEL_LC_26_76:
            return init_quectel_lc_26_76();
        default:
            return false;
    }
#else
    return simconnect_ready();
#endif // !SIMCONNECT
}

void gps_update() {
#if !SIMCONNECT
    char *line = uart_read(TX, RX);
    while (line) {
        bool continueProcessing = process_nmea_sentence(line);
        free(line);
        if (!continueProcessing) {
            return;
        }
        line = uart_read(TX, RX);
    }
#else
    gps.lat = scGPS.lat;
    gps.lng = scGPS.lng;
    gps.alt = (i32)scGPS.alt;
    gps.speed = scGPS.speed;
    gps.track = scGPS.track;
    // Not simulated
    gps.pdop = 1.f;
    gps.hdop = 1.f;
    gps.vdop = 1.f;
    gps.sats = 10;
#endif // !SIMCONNECT
    aircraft_set_gps_safe(data_valid(gps.lat, gps.lng, gps.alt, gps.speed, gps.track, gps.pdop, gps.hdop, gps.vdop));
}

void gps_calibrate_alt_offset(u32 num_samples) {
    log_message(TYPE_INFO, "Calibrating altitude", 1000, 100, false);
    u32 samples = 0;
    i64 alts = 0;
    while (samples < num_samples) {
        // GPS will be updated by runtime, we will check back in every second for a new altitude
        runtime_sleep_ms(1000, false);
        alts += gps.alt;
        samples++;
    }
    log_clear(TYPE_INFO);

    gps.altOffset = (i32)(alts / samples);
    printsys(gps, "altitude offset calculated as: %ld", gps.altOffset);
    gps.altOffsetCalibrated = true;
}

bool gps_is_supported() {
    return ((GPSCommandType)config.sensors.gpsCommandType != GPS_COMMAND_TYPE_NONE);
}

// clang-format off
GPS gps = {
    .lat = -200.0,
    .lng = -200.0,
    .alt = -1,
    .speed = -1.0f,
    .track = -1.0f,
    .pdop = -1.0f,
    .hdop = -1.0f,
    .vdop = -1.0f,
    .sats = -1,
    .altOffset = 0,
    .altOffsetCalibrated = false,
    .init = gps_init,
    .update = gps_update,
    .calibrate_alt_offset = gps_calibrate_alt_offset,
    .is_supported = gps_is_supported
};
// clang-format on
