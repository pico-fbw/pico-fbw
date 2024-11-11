/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

#if SIMCONNECT

#include <windows.h>
#include <stdio.h>
#include <SimConnect.h>

#include "simconnect.h"

HANDLE hSimConnect = NULL;

BOOL simconnect_init() {
    printf("[MSFS] simconnect is attempting a connection...\n");
    HRESULT hr = SimConnect_Open(&hSimConnect, "pico-fbw", NULL, 0, 0, 0);
    if (hr != S_OK) {
        printf("[MSFS] WARNING: failed to connect!\n");
        return FALSE;
    }
    printf("[MSFS] successfully connected to simulator\n");
    return TRUE;
}

void simconnect_deinit() {
    if (hSimConnect != NULL) {
        printf("[MSFS] closing connection\n");
        SimConnect_Close(hSimConnect);
        Sleep(500);
        hSimConnect = NULL;
    }
}

#endif // SIMCONNECT
