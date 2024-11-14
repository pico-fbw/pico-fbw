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

#include "platform/simconnect.h"

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
    ID_SC_AIL,
    ID_SC_ELE,
    ID_SC_RUD,
    ID_SC_NUM_ENG,
    // More data definitions will be created at runtime for engine throttle levels
};

HANDLE hSimConnect = nullptr;
SC_IMU scIMU;
SC_GPS scGPS;
i32 numEngines = 0; // Will be filled in later

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

/**
 * Converts a servo degree range (0-180) to a SimConnect `position` range (-1.0 to 1.0).
 * @param deg the servo degree value
 * @return the equivalent SimConnect `position` value
 */
static inline f32 deg_to_position(f32 deg) {
    return mapf(deg, 0.f, 180.f, -1.f, 1.f);
}

/**
 * Sets a control surface position.
 * @param deg the desired position in degrees (0-180)
 * @param id the SimConnect data definition ID to set
 * @return true if the data was sent successfully
 */
static bool set_control_surface(f32 deg, DataDefinitionRequestID id) {
    if (!hSimConnect)
        return false;
    f32 pos = deg_to_position(deg);
    HRESULT hr = SimConnect_SetDataOnSimObject(hSimConnect, id, SIMCONNECT_OBJECT_ID_USER, 0, 0, sizeof(pos), &pos);
    return hr == S_OK;
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
        case ID_SC_NUM_ENG:
            numEngines = *(i32 *)&pData->dwData;
            printmsfs("detected %d %s", numEngines, numEngines == 1 ? "engine" : "engines");
            // Now we can add all engine throttle level positions to the data definition
            for (i32 i = 1; i <= numEngines; i++) {
                char name[64];
                snprintf(name, sizeof(name), "GENERAL ENG THROTTLE LEVER POSITION:%d", i);
                SimConnect_AddToDataDefinition(hSimConnect, ID_SC_NUM_ENG + i, name, "percent", SIMCONNECT_DATATYPE_FLOAT32);
            }
            break;
        default:
            printmsfs("WARNING: unhandled SIMCONNECT_RECV_SIMOBJECT_DATA request ID %lu", pData->dwRequestID);
            break;
    }
    (void)pContext;
}

bool simconnect_init() {
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
    // Configure for control surface signals
    SimConnect_AddToDataDefinition(hSimConnect, ID_SC_AIL, "AILERON POSITION", "position", SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(hSimConnect, ID_SC_ELE, "ELEVATOR POSITION", "position", SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(hSimConnect, ID_SC_RUD, "RUDDER POSITION", "position", SIMCONNECT_DATATYPE_FLOAT32);
    // Request how many engines are present, SimConnect will get back to us with the number
    SimConnect_AddToDataDefinition(hSimConnect, ID_SC_NUM_ENG, "NUMBER OF ENGINES", "number", SIMCONNECT_DATATYPE_INT32);
    hr = SimConnect_RequestDataOnSimObject(hSimConnect, ID_SC_NUM_ENG, ID_SC_NUM_ENG, SIMCONNECT_OBJECT_ID_USER,
                                           SIMCONNECT_PERIOD_ONCE);
    if (hr != S_OK) {
        printmsfs("WARNING: failed to configure SC_NUM_ENG data! (%ld)", hr);
        return false;
    }
    printmsfs("configured all data requests");
    return true;
}

bool simconnect_ready() {
    return hSimConnect != nullptr;
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

bool simconnect_set_ail(f32 ail) {
    return set_control_surface(ail, ID_SC_AIL);
}

bool simconnect_set_ele(f32 ele) {
    return set_control_surface(ele, ID_SC_ELE);
}

bool simconnect_set_rud(f32 rud) {
    return set_control_surface(rud, ID_SC_RUD);
}

bool simconnect_set_thr(f32 thr) {
    if (!hSimConnect || numEngines < 1)
        return false;
    // Set throttle positions for all engines
    for (i32 i = 1; i <= numEngines; i++) {
        char name[64];
        snprintf(name, sizeof(name), "GENERAL ENG THROTTLE LEVER POSITION:%d", i);
        HRESULT hr =
            SimConnect_SetDataOnSimObject(hSimConnect, ID_SC_NUM_ENG + i, SIMCONNECT_OBJECT_ID_USER, 0, 0, sizeof(thr), &thr);
        if (hr != S_OK)
            return false;
    }
    return true;
}

void simconnect_deinit() {
    if (!hSimConnect)
        return;
    printmsfs("closing connection");
    SimConnect_Close(hSimConnect);
    hSimConnect = nullptr;
}

#endif // SIMCONNECT
