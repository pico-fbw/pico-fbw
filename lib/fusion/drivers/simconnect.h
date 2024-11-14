#pragma once

#include <stdbool.h>
#include "platform/types.h"

#include "drivers.h"

#if SIMCONNECT

bool simconnect_detect(byte addr, void *state);

bool simconnect_acc_read(Accelerometer *dev, void *state);

bool simconnect_gyro_read(Gyroscope *dev, void *state);

#endif // SIMCONNECT
