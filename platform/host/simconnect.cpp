/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

#if SIMCONNECT

// clang-format off

#include <windows.h>
#include <SimConnect.h>
#include <stdio.h>
#include <string.h>

#include "platform/types.h"

#include "simconnect.h"

/**
 * printf wrapper with a [MSFS] prefix
 * @param ... the format string and arguments to print (same as printf)
 * @note This function automatically appends a newline.
 */
#define printmsfs(...)                                                                                                         \
    printf("\033[38;2;0;181;255m[MSFS]\x1b[0m ");                                                                              \
    printf(__VA_ARGS__);                                                                                                       \
    printf("\n");

// clang-format on

// SimConnect definition/request IDs for our custom data requests (will be created on connection)
enum DataRequestID {
    EMU_AAHRS = 1,
    EMU_GPS,
};

HANDLE hSimConnect = nullptr;
EmuAAHRS emuAAHRS;
EmuGPS emuGPS;

// SimConnect callback. Will be called on a SIMCONNECT_RECV_OPEN message.
static void on_SIMCONNECT_RECV_OPEN(SIMCONNECT_RECV_OPEN *pData, void *pContext) {
    printmsfs("sim accepted connection");
    printmsfs("sim version: %lu.%lu.%lu.%lu", pData->dwApplicationVersionMajor, pData->dwApplicationVersionMinor,
              pData->dwApplicationBuildMajor, pData->dwApplicationBuildMinor);
    printmsfs("simconnect version: %lu.%lu.%lu.%lu", pData->dwSimConnectVersionMajor, pData->dwSimConnectVersionMinor,
              pData->dwSimConnectBuildMajor, pData->dwSimConnectBuildMinor);
    (void)pContext;
}

// SimConnect callback. Will be called on a SIMCONNECT_RECV_SIMOBJECT_DATA message.
static void on_SIMCONNECT_RECV_SIMOBJECT_DATA(SIMCONNECT_RECV_SIMOBJECT_DATA *pData, void *pContext) {
    switch (pData->dwRequestID) {
        case EMU_AAHRS: {
            memcpy(&emuAAHRS, &pData->dwData, sizeof(EmuAAHRS));
            // Invert roll and pitch because MSFS uses a different convention than pico-fbw
            emuAAHRS.roll = -emuAAHRS.roll;
            emuAAHRS.pitch = -emuAAHRS.pitch;
            break;
        }
        case EMU_GPS: {
            memcpy(&emuGPS, &pData->dwData, sizeof(EmuGPS));
            break;
        }
        default:
            break;
    }
    (void)pContext;
}

BOOL simconnect_init() {
    printmsfs("attempting to connect to simulator...");
    HRESULT hr = SimConnect_Open(&hSimConnect, "pico-fbw", nullptr, 0, 0, 0);
    if (hr != S_OK) {
        printmsfs("WARNING: failed to connect! (%ld)", hr);
        return false;
    }
    printmsfs("connection established, now configuring");
    // Configure data definitions for emulated AAHRS
    SimConnect_AddToDataDefinition(hSimConnect, EMU_AAHRS, "PLANE BANK DEGREES", "degrees", SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(hSimConnect, EMU_AAHRS, "PLANE PITCH DEGREES", "degrees", SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(hSimConnect, EMU_AAHRS, "PLANE HEADING DEGREES MAGNETIC", "degrees",
                                   SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(hSimConnect, EMU_AAHRS, "STRUCT WORLD ACCELERATION", "Gforce", SIMCONNECT_DATATYPE_XYZ);
    SimConnect_AddToDataDefinition(hSimConnect, EMU_AAHRS, "STRUCT BODY ROTATION VELOCITY", "degrees per second",
                                   SIMCONNECT_DATATYPE_XYZ);
    SimConnect_AddToDataDefinition(hSimConnect, EMU_AAHRS, "INDICATED ALTITUDE", "feet", SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_RequestDataOnSimObject(hSimConnect, EMU_AAHRS, EMU_AAHRS, SIMCONNECT_OBJECT_ID_USER,
                                      SIMCONNECT_PERIOD_SIM_FRAME);
    // Configure for emulated GPS
    SimConnect_AddToDataDefinition(hSimConnect, EMU_GPS, "PLANE LATITUDE", "degrees");
    SimConnect_AddToDataDefinition(hSimConnect, EMU_GPS, "PLANE LONGITUDE", "degrees");
    SimConnect_AddToDataDefinition(hSimConnect, EMU_GPS, "PLANE ALTITUDE", "feet", SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(hSimConnect, EMU_GPS, "GPS GROUND SPEED", "knots", SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(hSimConnect, EMU_GPS, "GPS GROUND TRUE HEADING", "degrees", SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_RequestDataOnSimObject(hSimConnect, EMU_GPS, EMU_GPS, SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_SIM_FRAME);
    printmsfs("configured all data requests");
    return true;
}

void simconnect_poll() {
    if (!hSimConnect)
        return;
    SimConnect_CallDispatch(
        hSimConnect,
        [](SIMCONNECT_RECV *pData, DWORD cbData, void *pContext) -> void {
            switch (pData->dwID) {
                case SIMCONNECT_RECV_ID_OPEN:
                    on_SIMCONNECT_RECV_OPEN((SIMCONNECT_RECV_OPEN *)pData, pContext);
                    break;
                case SIMCONNECT_RECV_ID_SIMOBJECT_DATA:
                    on_SIMCONNECT_RECV_SIMOBJECT_DATA((SIMCONNECT_RECV_SIMOBJECT_DATA *)pData, pContext);
                    break;
                default:
                    printmsfs("WARNING: unhandled message %lu", pData->dwID);
                    break;
            }
            (void)cbData;
            (void)pContext;
        },
        nullptr);
}

void simconnect_deinit() {
    if (!hSimConnect)
        return;
    printmsfs("closing connection");
    SimConnect_Close(hSimConnect);
    hSimConnect = nullptr;
}

#endif // SIMCONNECT
