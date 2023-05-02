/*
 * This Library is ported from the AutotunerPID Toolkit by William Spinelli
 * (http://www.mathworks.com/matlabcentral/fileexchange/4652) 
 * Copyright (c) 2004
 *
 * This Library is licensed under the BSD License:
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are 
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright 
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright 
 *       notice, this list of conditions and the following disclaimer in 
 *       the documentation and/or other materials provided with the distribution
 *       
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
*/ 
 
/*
 * Credit to "br3ttb" "t0mpr1c3" and for developing the Arduino PID autotune library that the auto tuning is based off.
 * https://github.com/t0mpr1c3/Arduino-PID-AutoTune-Library
*/

#include <stdbool.h>
#include "pico/stdlib.h"

#include "../config.h"

#include "pidtune.h"

float *input;
float *output;
float setpoint;

float oStep;
float noiseBand;
unsigned char nLookBack;
unsigned char controlType; // selects autotune algorithm

enum AutoTunerState state; // state of autotuner finite state machine
uint64_t lastTime;
uint64_t sampleTime;
enum Peak peakType;
uint64_t lastPeakTime[5]; // peak time, most recent in array element 0
float lastPeaks[5]; // peak value, most recent in array element 0
unsigned char peakCount;
float lastInputs[101]; // process values, most recent in array element 0
unsigned char inputCount;
float outputStart;
float workingNoiseBand;
float workingOstep;
float inducedAmplitude;
float Kp, Ti, Td;
  
#ifdef AUTOTUNE_RELAY_BIAS  
    float relayBias;
    unsigned long lastStepTime[5]; // step time, most recent in array element 0
    float sumInputSinceLastStep[5]; // integrated process values, most recent in array element 0
    unsigned char stepCount;
#endif


void pidtune_init(float* input, float* output) {
    
}

void pidtune_cancel() {

}

bool pidtune_runtime() {

}

float pidtune_processValueOffset(float avgStep1, float avgStep2) {

}

void pidtune_setOutputStep(float step) {

}

void pidtune_setControlType(unsigned char type) {

}

void pidtune_setNoiseBand(float band) {

}

void pidtune_setLookbackSec(int sec) {

}

float pidtune_getKp() {

}

float pidtune_getKi() {

}

float pidtune_getKd() {

}
