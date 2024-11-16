#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "platform/types.h"

#include "drivers.h"

#if SIMCONNECT

    #define SC_SCALE_FACTOR INT16_MAX

bool simconnect_detect(byte addr, void *state);

bool simconnect_acc_read(Accelerometer *dev, void *state);

bool simconnect_gyro_read(Gyroscope *dev, void *state);

#endif // SIMCONNECT
