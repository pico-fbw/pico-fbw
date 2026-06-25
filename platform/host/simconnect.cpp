/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
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
    ID_AIL_OUT,
    ID_ELE_OUT,
    ID_RUD_OUT,
    ID_NUM_ENG,
    // More data definitions will be created at runtime for engine throttle levels
};

// SimConnect notification group IDs
enum NotificationGroup {
    GROUP_FCTRL = 1,
};

// SimConnect event IDs
enum EventID {
    EVENT_AIL_SET = 1,
    EVENT_ELE_SET,
    EVENT_RUD_SET,
    EVENT_THR_SET,
};

HANDLE hSimConnect = nullptr;
SC_IMU scIMU = {
    .roll = INFINITY,
    .pitch = INFINITY,
    .yaw = INFINITY,
    .bodyAccel = {0.0, 0.0, 0.0},
    .gyro = {0.0, 0.0, 0.0},
    .alt = -1.f,
    .accel = {0.0, 0.0, 0.0},
};
SC_GPS scGPS = {
    .lat = -200.0,
    .lng = -200.0,
    .alt = -1.f,
    .speed = -1.f,
    .track = -1.f,
};
i32 numEngines = 0; // Will be filled in later

f32 ailPos = 0.f, elePos = 0.f, rudPos = 0.f, thrPos = 0.f; // Last retrieved control surface positions

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
    for (u32 i = 0; i < count_of(imu->accel); i++) {
        imu->accel[i] = (imu->bodyAccel[i] + g[i]) / GRAVITY;
    }
}

/**
 * Converts a SimConnect `position` range (-1.0 to 1.0) to a servo degree range (0-180).
 * @param pos the SimConnect `position` value
 * @return the equivalent servo degree value
 */
static inline f32 position_to_deg(f32 pos) {
    return mapf(pos, -1.f, 1.f, 0.f, 180.f);
}

/**
 * Converts a SimConnect `position` range returned by an event (-16383 to 16384) to an `f32` normalized position range
 * (-1.0 to 1.0).
 * @param data the event data to convert
 * @return the equivalent normalized position value
 */
static inline f32 eventdata_to_position(DWORD data) {
    return (f32)((i32)data) / 16384.0f;
}

/**
 * Sets a control surface position.
 * @param deg the desired position in degrees (0-180)
 * @param id the SimConnect data definition ID to set
 * @return true if the data was sent successfully
 */
static bool set_control_surface(f32 deg, DataDefinitionRequestID id) {
    if (!hSimConnect) {
        return false;
    }
    f32 pos = mapf(deg, 0.f, 180.f, -1.f, 1.f); // Map from servo degree range to SimConnect position range
    return SimConnect_SetDataOnSimObject(hSimConnect, id, SIMCONNECT_OBJECT_ID_USER, 0, 0, sizeof(pos), &pos) == S_OK;
}

/**
 * Sets the throttle level for all engines.
 * @param thr the desired throttle level (0-100)
 * @return true if the data was sent successfully
 */
static bool set_throttle(f32 thr) {
    if (!hSimConnect || numEngines < 1) {
        return false;
    }
    // Set throttle positions for all engines
    for (i32 i = 1; i <= numEngines; i++) {
        char name[64];
        snprintf(name, sizeof(name), "GENERAL ENG THROTTLE LEVER POSITION:%d", i);
        HRESULT hr = SimConnect_SetDataOnSimObject(hSimConnect, ID_NUM_ENG + i, SIMCONNECT_OBJECT_ID_USER, 0, 0,
                                                   sizeof(thr), &thr);
        if (FAILED(hr)) {
            return false;
        }
    }
    return true;
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
            // Roll and pitch must be inverted as MSFS uses a different convention than pico-fbw
            scIMU.roll = -scIMU.roll;
            scIMU.gyro[2] = -scIMU.gyro[2]; // Roll = Z axis in MSFS
            scIMU.pitch = -scIMU.pitch;
            scIMU.gyro[0] = -scIMU.gyro[0]; // Pitch = X axis in MSFS
            // Simulate accelerometer readings as the fusion system expects them
            simulate_accel(&scIMU);
            break;
        case ID_SC_GPS:
            memcpy(&scGPS, &pData->dwData, sizeof(SC_GPS));
            break;
        case ID_NUM_ENG:
            numEngines = *(i32 *)&pData->dwData;
            printmsfs("detected %d %s", numEngines, numEngines == 1 ? "engine" : "engines");
            // Now we can add all engine throttle level positions to the data definition
            for (i32 i = 1; i <= numEngines; i++) {
                char name[64];
                snprintf(name, sizeof(name), "GENERAL ENG THROTTLE LEVER POSITION:%d", i);
                SimConnect_AddToDataDefinition(hSimConnect, ID_NUM_ENG + i, name, "percent",
                                               SIMCONNECT_DATATYPE_FLOAT32);
            }
            break;
        default:
            printmsfs("WARNING: unhandled SIMCONNECT_RECV_SIMOBJECT_DATA request ID %lu", pData->dwRequestID);
            break;
    }
    (void)pContext;
}

static void on_SIMCONNECT_RECV_EVENT(SIMCONNECT_RECV_EVENT *pData, void *pContext) {
    switch (pData->uEventID) {
        // Reverse signs on control surface positions, also to match convention
        case EVENT_AIL_SET: {
            ailPos = -eventdata_to_position(pData->dwData);
            break;
        }
        case EVENT_ELE_SET: {
            elePos = -eventdata_to_position(pData->dwData);
            break;
        }
        case EVENT_RUD_SET: {
            rudPos = -eventdata_to_position(pData->dwData);
            break;
        }
        case EVENT_THR_SET: {
            thrPos = mapf((f32)pData->dwData, 0.f, 16384.f, 0.f, 100.f);
            break;
        }
        default:
            printmsfs("WARNING: unhandled event ID %lu", pData->uEventID);
            break;
    }
    (void)pContext;
}

/**
 * Configures the data definition for the emulated IMU.
 */
static bool configure_datadef_sc_imu() {
    SimConnect_AddToDataDefinition(hSimConnect, ID_SC_IMU, "PLANE BANK DEGREES", "degrees");
    SimConnect_AddToDataDefinition(hSimConnect, ID_SC_IMU, "PLANE PITCH DEGREES", "degrees");
    SimConnect_AddToDataDefinition(hSimConnect, ID_SC_IMU, "PLANE HEADING DEGREES MAGNETIC", "degrees");
    SimConnect_AddToDataDefinition(hSimConnect, ID_SC_IMU, "ACCELERATION BODY X", "meters per second squared");
    SimConnect_AddToDataDefinition(hSimConnect, ID_SC_IMU, "ACCELERATION BODY Y", "meters per second squared");
    SimConnect_AddToDataDefinition(hSimConnect, ID_SC_IMU, "ACCELERATION BODY Z", "meters per second squared");
    // dps is not available, so we use rad/s and convert later
    SimConnect_AddToDataDefinition(hSimConnect, ID_SC_IMU, "STRUCT BODY ROTATION VELOCITY", "radians per second",
                                   SIMCONNECT_DATATYPE_XYZ);
    SimConnect_AddToDataDefinition(hSimConnect, ID_SC_IMU, "INDICATED ALTITUDE", "feet", SIMCONNECT_DATATYPE_FLOAT32);
    return SUCCEEDED(SimConnect_RequestDataOnSimObject(hSimConnect, ID_SC_IMU, ID_SC_IMU, SIMCONNECT_OBJECT_ID_USER,
                                                       SIMCONNECT_PERIOD_SIM_FRAME));
}

/**
 * Configures the data definition for the emulated GPS.
 */
static bool configure_datadef_sc_gps() {
    SimConnect_AddToDataDefinition(hSimConnect, ID_SC_GPS, "PLANE LATITUDE", "degrees");
    SimConnect_AddToDataDefinition(hSimConnect, ID_SC_GPS, "PLANE LONGITUDE", "degrees");
    SimConnect_AddToDataDefinition(hSimConnect, ID_SC_GPS, "PLANE ALTITUDE", "feet", SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(hSimConnect, ID_SC_GPS, "GPS GROUND SPEED", "knots", SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(hSimConnect, ID_SC_GPS, "GPS GROUND TRUE HEADING", "degrees",
                                   SIMCONNECT_DATATYPE_FLOAT32);
    // GPS data is updated every second (to simulate real GPS modules being somewhat slow)
    return SUCCEEDED(SimConnect_RequestDataOnSimObject(hSimConnect, ID_SC_GPS, ID_SC_GPS, SIMCONNECT_OBJECT_ID_USER,
                                                       SIMCONNECT_PERIOD_SECOND));
}

/**
 * Configures the data definitions for the control surface signals.
 */
static bool configure_datadef_control_surfaces() {
    SimConnect_AddToDataDefinition(hSimConnect, ID_AIL_OUT, "AILERON POSITION", "position",
                                   SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(hSimConnect, ID_ELE_OUT, "ELEVATOR POSITION", "position",
                                   SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(hSimConnect, ID_RUD_OUT, "RUDDER POSITION", "position", SIMCONNECT_DATATYPE_FLOAT32);
    return true;
}

/**
 * Configures the event mapping for the flight control surfaces.
 */
static bool configure_event_fctrl() {
    SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_AIL_SET, "AXIS_AILERONS_SET");
    SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_ELE_SET, "AXIS_ELEVATOR_SET");
    SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_RUD_SET, "AXIS_RUDDER_SET");
    SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_THR_SET, "THROTTLE1_SET");
    for (u32 event = EVENT_AIL_SET; event <= EVENT_THR_SET; event++) {
        SimConnect_AddClientEventToNotificationGroup(hSimConnect, GROUP_FCTRL, event, true);
    }
    return SUCCEEDED(
        SimConnect_SetNotificationGroupPriority(hSimConnect, GROUP_FCTRL, SIMCONNECT_GROUP_PRIORITY_HIGHEST_MASKABLE));
}

bool simconnect_init() {
    printmsfs("attempting to connect to simulator...");
    HRESULT hr = SimConnect_Open(&hSimConnect, "pico-fbw", nullptr, 0, 0, 0);
    if (FAILED(hr)) {
        printmsfs("WARNING: failed to connect! (%ld)", hr);
        return false;
    }
    printmsfs("connection established, now configuring");
    // Configure data definitions
    if (!configure_datadef_sc_imu()) {
        printmsfs("WARNING: failed to configure SC_IMU data!");
        return false;
    }
    if (!configure_datadef_sc_gps()) {
        printmsfs("WARNING: failed to configure SC_GPS data!");
        return false;
    }
    if (!configure_datadef_control_surfaces()) {
        printmsfs("WARNING: failed to configure control surface data!");
        return false;
    }
    // Configure events
    if (!configure_event_fctrl()) {
        printmsfs("WARNING: failed to configure flight control events!");
        return false;
    }
    // Request how many engines are present, SimConnect will get back to us with the number
    // This is needed to later set throttle levels for all engines
    SimConnect_AddToDataDefinition(hSimConnect, ID_NUM_ENG, "NUMBER OF ENGINES", "number", SIMCONNECT_DATATYPE_INT32);
    hr = SimConnect_RequestDataOnSimObject(hSimConnect, ID_NUM_ENG, ID_NUM_ENG, SIMCONNECT_OBJECT_ID_USER,
                                           SIMCONNECT_PERIOD_ONCE);
    if (FAILED(hr)) {
        printmsfs("WARNING: failed to get number of engines!");
        return false;
    }
    printmsfs("configured all data requests");
    return true;
}

bool simconnect_ready() {
    return hSimConnect != nullptr;
}

void simconnect_poll() {
    if (!hSimConnect) {
        return;
    }
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
                case SIMCONNECT_RECV_ID_EVENT:
                    on_SIMCONNECT_RECV_EVENT((SIMCONNECT_RECV_EVENT *)pData, pContext);
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

bool simconnect_set(SCFlightControl fctrl, f32 val) {
    switch (fctrl) {
        case FCTRL_AIL:
            return set_control_surface(val, ID_AIL_OUT);
        case FCTRL_ELE:
            return set_control_surface(val, ID_ELE_OUT);
        case FCTRL_RUD:
            return set_control_surface(val, ID_RUD_OUT);
        case FCTRL_THR:
            return set_throttle(val);
    }
    return false;
}

f32 simconnect_get(SCFlightControl fctrl) {
    switch (fctrl) {
        case FCTRL_AIL:
            return position_to_deg(ailPos);
        case FCTRL_ELE:
            return position_to_deg(elePos);
        case FCTRL_RUD:
            return position_to_deg(rudPos);
        case FCTRL_THR:
            return thrPos;
    }
    return 0.f;
}

void simconnect_deinit() {
    if (!hSimConnect) {
        return;
    }
    printmsfs("closing connection");
    SimConnect_Close(hSimConnect);
    hSimConnect = nullptr;
}

#endif // SIMCONNECT
