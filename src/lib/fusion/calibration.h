#ifndef __CALIBRATON_H
#define __CALIBRATON_H

#include <stdbool.h>
#include "fusion.h"

// Floats, not bytes
// Changing this WILL resize the system's flash FS, so beware! It will likely cause data corruption if not handled correctly.
// Must also be aligned to FLOAT_SECTOR_SIZE
#define FUSION_CALIBRATION_STORAGE_SIZE 64

bool GetMagCalibrationFromFlash(float *cal_values);
bool GetGyroCalibrationFromFlash(float *cal_values);
bool GetAccelCalibrationFromFlash(float *cal_values);
void SaveMagCalibrationToFlash(SensorFusionGlobals *sfg);
void SaveGyroCalibrationToFlash(SensorFusionGlobals *sfg);
void SaveAccelCalibrationToFlash(SensorFusionGlobals *sfg);
bool MagCalibrationExists();
bool GyroCalibrationExists();
bool AccelCalibrationExists();

#endif // __CALIBRATON_H