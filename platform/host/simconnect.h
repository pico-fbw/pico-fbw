#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#if SIMCONNECT

/**
 * Initializes a connection to the MSFS SimConnect API.
 * @return TRUE if the connection was successful
 */
BOOL simconnect_init();

/**
 * Polls the MSFS SimConnect API for new messages.
 */
void simconnect_poll();

/**
 * Closes an active connection to the MSFS SimConnect API if one exists.
 */
void simconnect_deinit();

#pragma pack(push, 1) // Pack structs for compatibility with SimConnect
// Emulated (SimConnect) AAHRS data definition
typedef struct EmuAAHRS {
    f32 roll; // deg
    f32 pitch; // deg
    f32 yaw; // deg
    f64 accel[3]; // [X, Y, Z], g
    f64 gyro[3]; // [X, Y, Z], deg/s
    f32 alt; // ft
} EmuAAHRS;

// Emulated (SimConnect) GPS data definition
typedef struct EmuGPS {
    f64 lat; // deg
    f64 lng; // deg
    f32 alt; // ft
    f32 speed; // kts
    f32 track; // deg
} EmuGPS;
#pragma pack(pop)

extern EmuAAHRS emuAAHRS;
extern EmuGPS emuGPS;

// clang-format off
#ifdef __cplusplus
}
#endif
// clang-format on

#endif // SIMCONNECT
