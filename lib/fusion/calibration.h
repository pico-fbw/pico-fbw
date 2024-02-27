#pragma once

#include <stdbool.h>
#include "fusion.h"

bool GetMagCalibrationFromFlash(float *cal_values);
bool GetGyroCalibrationFromFlash(float *cal_values);
bool GetAccelCalibrationFromFlash(float *cal_values);
void SaveMagCalibrationToFlash(SensorFusionGlobals *sfg);
void SaveGyroCalibrationToFlash(SensorFusionGlobals *sfg);
void SaveAccelCalibrationToFlash(SensorFusionGlobals *sfg);
void EraseFusionCalibration();
bool MagCalibrationExists();
bool GyroCalibrationExists();
bool AccelCalibrationExists();
