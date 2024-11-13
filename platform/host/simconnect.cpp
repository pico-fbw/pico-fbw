/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

#if SIMCONNECT

// clang-format off

#include <math.h>
#include <windows.h>
#include <SimConnect.h>
#include <stdio.h>
#include <string.h>

#include "platform/helpers.h"

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

#define GRAVITY 9.81

// clang-format on

// SimConnect definition/request IDs for our custom data requests (will be created on connection)
enum DataDefinitionRequestID {
    ID_SC_IMU = 1,
    ID_SC_GPS,
};

HANDLE hSimConnect = nullptr;
SC_IMU scIMU;
SC_GPS scGPS;

/**
 * Simulates readings from a MEMS accelerometer based on available SimConnect data.
 * @param imu the IMU data to simulate/modify
 */
static void simulate_accel(SC_IMU *imu) {
    f64 roll = radians(imu->roll);
    f64 pitch = radians(imu->pitch);
    // Compute gravity vector in aircraft body frame
    f64 g[3] = {
        GRAVITY * sin(pitch),
        -GRAVITY * sin(roll) * cos(pitch),
        -GRAVITY * cos(roll) * cos(pitch),
    };
    // Combine linear (body) acceleration with gravity and convert to G-force
    for (u32 i = 0; i < count_of(imu->accel); i++)
        imu->accel[i] = (imu->bodyAccel[i] + g[i]) / GRAVITY;
}

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
        case ID_SC_IMU:
            // Don't copy accel[] as that will be simulated
            memcpy(&scIMU, &pData->dwData, sizeof(SC_IMU) - sizeof(scIMU.accel));
            // Simulate accelerometer readings as the fusion system expects them
            simulate_accel(&scIMU);
            break;
        case ID_SC_GPS:
            memcpy(&scGPS, &pData->dwData, sizeof(SC_GPS));
            break;
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
    // Configure data definitions for emulated IMU
    SimConnect_AddToDataDefinition(hSimConnect, ID_SC_IMU, "PLANE BANK DEGREES", "degrees");
    SimConnect_AddToDataDefinition(hSimConnect, ID_SC_IMU, "PLANE PITCH DEGREES", "degrees");
    SimConnect_AddToDataDefinition(hSimConnect, ID_SC_IMU, "PLANE HEADING DEGREES MAGNETIC", "degrees");
    SimConnect_AddToDataDefinition(hSimConnect, ID_SC_IMU, "ACCELERATION BODY X", "meters per second squared");
    SimConnect_AddToDataDefinition(hSimConnect, ID_SC_IMU, "ACCELERATION BODY Y", "meters per second squared");
    SimConnect_AddToDataDefinition(hSimConnect, ID_SC_IMU, "ACCELERATION BODY Z", "meters per second squared");
    SimConnect_AddToDataDefinition(hSimConnect, ID_SC_IMU, "STRUCT BODY ROTATION VELOCITY", "degrees per second",
                                   SIMCONNECT_DATATYPE_XYZ);
    SimConnect_AddToDataDefinition(hSimConnect, ID_SC_IMU, "INDICATED ALTITUDE", "feet", SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_RequestDataOnSimObject(hSimConnect, ID_SC_IMU, ID_SC_IMU, SIMCONNECT_OBJECT_ID_USER,
                                           SIMCONNECT_PERIOD_SIM_FRAME);
    if (hr != S_OK) {
        printmsfs("WARNING: failed to configure SC_IMU data! (%ld)", hr);
        return false;
    }
    // Configure for emulated GPS
    SimConnect_AddToDataDefinition(hSimConnect, ID_SC_GPS, "PLANE LATITUDE", "degrees");
    SimConnect_AddToDataDefinition(hSimConnect, ID_SC_GPS, "PLANE LONGITUDE", "degrees");
    SimConnect_AddToDataDefinition(hSimConnect, ID_SC_GPS, "PLANE ALTITUDE", "feet", SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(hSimConnect, ID_SC_GPS, "GPS GROUND SPEED", "knots", SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(hSimConnect, ID_SC_GPS, "GPS GROUND TRUE HEADING", "degrees", SIMCONNECT_DATATYPE_FLOAT32);
    // GPS data is updated every second (to simulate real GPS modules being somewhat slow)
    hr = SimConnect_RequestDataOnSimObject(hSimConnect, ID_SC_GPS, ID_SC_GPS, SIMCONNECT_OBJECT_ID_USER,
                                           SIMCONNECT_PERIOD_SECOND);
    if (hr != S_OK) {
        printmsfs("WARNING: failed to configure SC_GPS data! (%ld)", hr);
        return false;
    }
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
