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
 * Closes an active connection to the MSFS SimConnect API if one exists.
 */
void simconnect_deinit();

#ifdef __cplusplus
}
#endif

#endif // SIMCONNECT
