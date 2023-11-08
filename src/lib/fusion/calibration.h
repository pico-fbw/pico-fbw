#ifndef __CALIBRATON_H
#define __CALIBRATON_H

#include "fusion.h"

bool GetMagCalibrationFromNVM( float *cal_values );
bool GetGyroCalibrationFromNVM( float *cal_values );
bool GetAccelCalibrationFromNVM( float *cal_values );
void SaveMagCalibrationToNVM(SensorFusionGlobals *sfg);
void SaveGyroCalibrationToNVM(SensorFusionGlobals *sfg);
void SaveAccelCalibrationToNVM(SensorFusionGlobals *sfg);
void EraseMagCalibrationFromNVM(void);
void EraseGyroCalibrationFromNVM(void);
void EraseAccelCalibrationFromNVM(void);

#endif // __CALIBRATON_H