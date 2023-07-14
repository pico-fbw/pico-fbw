// TODO: try using this PID library it might be better: https://github.com/jackw01/arduino-pid-autotuner

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
 * Credit to "br3ttb" and "t0mpr1c3" for developing the Arduino PID autotune library that the auto tuning is based off.
 * https://github.com/t0mpr1c3/Arduino-PID-AutoTune-Library
*/

/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "pico/time.h"

#include "pidtune.h"

#ifdef PID_AUTOTUNE
    double *input;
    double *output;
    double setpoint;

    double oStep;
    double noiseBand;
    unsigned char nLookBack;

    enum AutoTunerState aTuneState; // state of autotuner finite state machine
    uint64_t lastTime;
    uint64_t sampleTime;
    enum Peak peakType;
    uint64_t lastPeakTime[5]; // peak time, most recent in array element 0
    double lastPeaks[5]; // peak value, most recent in array element 0
    unsigned char peakCount;
    double lastInputs[101]; // process values, most recent in array element 0
    unsigned char inputCount;
    double outputStart;
    double workingNoiseBand;
    double workingOstep;
    double inducedAmplitude;
    uint16_t KpDiv, TiDiv, TdDiv;
    double Kp, Ti, Td = 0.0f;
    
    #ifdef AUTOTUNE_RELAY_BIAS  
        double relayBias;
        unsigned long lastStepTime[5]; // step time, most recent in array element 0
        double sumInputSinceLastStep[5]; // integrated process values, most recent in array element 0
        unsigned char stepCount;
    #endif


    void pidtune_init(double* Input, double* Output) {
        input = Input;
        output = Output;
        // Set default values, they can be overridden with other functions
        // (Normally we would specify a default control type but I have removed that from the original)
        noiseBand = 0.5;
        aTuneState = AUTOTUNER_OFF;
        oStep = 10.0;
        pidtune_setLookbackSec(10);
    }

    void pidtune_cancel() {
        aTuneState = AUTOTUNER_OFF;
    }

    bool pidtune_runtime() {
        // Get current time in ms (since Pico has been powered)
        uint64_t now = (time_us_64() / 1000);
        // If the autotuner hasn't been run before, initialize working variables the first time around
        if (aTuneState == AUTOTUNER_OFF) { 
            peakType = NOT_A_PEAK;
            inputCount = 0;
            peakCount = 0;
            setpoint = *input;
            outputStart = *output;
            lastPeakTime[0] = now;
            workingNoiseBand = noiseBand;
            workingOstep = oStep;
            aTuneState = RELAY_STEP_UP;

            #if defined (AUTOTUNE_RELAY_BIAS) 
                relayBias = 0.0;
                stepCount = 0;   
                lastStepTime[0] = now;
                sumInputSinceLastStep[0] = 0.0;
            #endif
        }
        else if ((now - lastTime) < sampleTime) {
            // Return false if we're not ready for a new input yet
            return false;
        }

        // Otherwise, get new input
        lastTime = now;
        double refVal = *input;

        #ifdef AUTOTUNE_RELAY_BIAS
            // used to calculate relay bias
            sumInputSinceLastStep[0] += refVal;
        #endif  

        bool justChanged = false; 
        // Check input and change relay aTuneState if necessary
        if ((aTuneState == RELAY_STEP_UP) && (refVal > setpoint + workingNoiseBand)) {
            aTuneState = RELAY_STEP_DOWN;
            justChanged = true;
        } else if ((aTuneState == RELAY_STEP_DOWN) && (refVal < setpoint - workingNoiseBand)) {
            aTuneState = RELAY_STEP_UP;
            justChanged = true;
        }
        if (justChanged) {
            #ifdef AUTOTUNE_RELAY_BIAS
                // check symmetry of oscillation
                // and introduce relay bias if necessary
                if (stepCount > 4) {
                    double avgStep1 = 0.5 * (double) ((lastStepTime[0] - lastStepTime[1]) + (lastStepTime[2] - lastStepTime[3]));
                    double avgStep2 = 0.5 * (double) ((lastStepTime[1] - lastStepTime[2]) + (lastStepTime[3] - lastStepTime[4]));
                    if ((avgStep1 > 1e-10) && (avgStep2 > 1e-10)) {
                        double asymmetry = (avgStep1 > avgStep2) ?
                                            (avgStep1 - avgStep2) / avgStep1 : (avgStep2 - avgStep1) / avgStep2;

                        if (asymmetry > AUTOTUNE_STEP_ASYMMETRY_TOLERANCE) {
                            /**
                             * Relay steps are asymmetric
                             * Calculate relay bias using:
                             * "Autotuning of PID Controllers: A Relay Feedback Approach", by Cheng-Ching Yu, 2nd Edition, equation 7.39, p. 148
                            */

                            // Calculate change in relay bias
                            double deltaRelayBias = - pidtune_processValueOffset(avgStep1, avgStep2) * workingOstep;
                            if (aTuneState == RELAY_STEP_DOWN) {
                                deltaRelayBias = -deltaRelayBias;
                            }
                            
                            if (abs(deltaRelayBias) > workingOstep * AUTOTUNE_STEP_ASYMMETRY_TOLERANCE) {
                                // Change is large enough to bother with
                                relayBias += deltaRelayBias;
                                /*
                                    // Adjust step height with respect to output limits
                                    // (commented out because the auto tuner does not necessarily know what the output limits are)
                                    // TODO: work on this? an issue with the library but still I can probably fix it
                                    double relayHigh = outputStart + workingOstep + relayBias;
                                    double relayLow  = outputStart - workingOstep + relayBias;
                                    if (relayHigh > outMax) {
                                        relayHigh = outMax;
                                    }
                                    if (relayLow  < outMin) {
                                        relayHigh = outMin;
                                    }
                                    workingOstep = 0.5 * (relayHigh - relayLow);
                                    relayBias = relayHigh - outputStart - workingOstep;
                                */

                                // Reset relay step counter to give the process value oscillation time to settle with the new relay bias value
                                stepCount = 0;
                            }
                        }
                    }
                }

                // shift step time and integrated process value arrays
                for (unsigned char i = (stepCount > 4 ? 4 : stepCount); i > 0; i--) {
                    lastStepTime[i] = lastStepTime[i - 1];
                    sumInputSinceLastStep[i] = sumInputSinceLastStep[i - 1];
                }
                stepCount++;
                lastStepTime[0] = now;
                sumInputSinceLastStep[0] = 0.0;

            #endif // ifdef AUTOTUNE_RELAY_BIAS
        } // if justChanged

        // Set output
        // TODO: respect output limits! (again, library fault but I can probably fix this)
        if (((unsigned char) aTuneState & (STEADY_STATE_AFTER_STEP_UP | RELAY_STEP_UP)) > 0) {
            #ifdef AUTOTUNE_RELAY_BIAS 
                *output = outputStart + workingOstep + relayBias;
            #else    
                *output = outputStart + workingOstep;
            #endif
        } else if (aTuneState == RELAY_STEP_DOWN) {
            #if defined (AUTOTUNE_RELAY_BIAS)    
                *output = outputStart - workingOstep + relayBias;
            #else
                *output = outputStart - workingOstep;
            #endif
        }

        // Store initial inputs
        // We don't want to trust the maxes or mins until the input array is full
        inputCount++;
        if (inputCount <= nLookBack) {
            lastInputs[nLookBack - inputCount] = refVal;
            return false;
        }

        // Shift array of process values and identify peaks
        inputCount = nLookBack;
        bool isMax = true;
        bool isMin = true;
        for (int i = inputCount - 1; i >= 0; i--) {
            double val = lastInputs[i];
            if (isMax) {
                isMax = (refVal >= val);
            }
            if (isMin)  {
                isMin = (refVal <= val);
            }
            lastInputs[i + 1] = val;
        }
        lastInputs[0] = refVal; 

        if (((unsigned char) aTuneState & (STEADY_STATE_AT_BASELINE | STEADY_STATE_AFTER_STEP_UP)) > 0) {
            // Check that all the recent inputs are equal give or take expected noise
            double iMax = lastInputs[0];
            double iMin = lastInputs[0];
            double avgInput = 0.0;
            for (unsigned char i = 0; i <= inputCount; i++) {
                double val = lastInputs[i];
                if (iMax < val) {
                    iMax = val;
                }
                if (iMin > val) {
                    iMin = val;
                }
                avgInput += val;
            } 
            avgInput /= (double)(inputCount + 1);

            // if recent inputs are stable
            if ((iMax - iMin) <= 2.0 * workingNoiseBand) {
                
                #ifdef AUTOTUNE_RELAY_BIAS   
                    lastStepTime[0] = now;
                #endif

                if (aTuneState == STEADY_STATE_AT_BASELINE) {
                    aTuneState = STEADY_STATE_AFTER_STEP_UP;
                    lastPeaks[0] = avgInput;  
                    inputCount = 0;
                    return false;
                }
                // else aTuneState == STEADY_STATE_AFTER_STEP_UP
                aTuneState = RELAY_STEP_DOWN;

                #ifdef AUTOTUNE_RELAY_BIAS
                    sumInputSinceLastStep[0] = 0.0;
                #endif

                return false;
            } else {
                return false;
            }
        }

        // Increment peak count and record peak time for both maxima and minima 
        justChanged = false;
        if (isMax) {
            if (peakType == MINIMUM) {
                justChanged = true;
            }
            peakType = MAXIMUM;
        } else if (isMin) {
            if (peakType == MAXIMUM) {
                justChanged = true;
            }
            peakType = MINIMUM;
        }

        // Update peak times and values
        if (justChanged) {
            peakCount++;
            // Shift peak time and peak value arrays
            for (unsigned char i = (peakCount > 4 ? 4 : peakCount); i > 0; i--) {
                lastPeakTime[i] = lastPeakTime[i - 1];
                lastPeaks[i] = lastPeaks[i - 1];
            }
        }
        if (isMax || isMin) {
            lastPeakTime[0] = now;
            lastPeaks[0] = refVal;
        }

        // Check for convergence of induced oscillation
        // Convergence of amplitude is assessed on last 4 peaks (1.5 cycles)
        double inducedAmplitude = 0.0;
        double phaseLag;
        if (
        #ifdef AUTOTUNE_RELAY_BIAS
            (stepCount > 4) &&
        #endif
        justChanged && 
        (peakCount > 4)
        ) { 
            double absMax = lastPeaks[1];
            double absMin = lastPeaks[1];
            for (unsigned char i = 2; i <= 4; i++) {
                double val = lastPeaks[i];
                inducedAmplitude += abs( val - lastPeaks[i - 1]); 
                if (absMax < val) {
                    absMax = val;
                }
                if (absMin > val) {
                    absMin = val;
                }
            }
            inducedAmplitude /= 6.0;

            // Check convergence criterion for amplitude of induced oscillation
            if (((0.5 * (absMax - absMin) - inducedAmplitude) / inducedAmplitude) < AUTOTUNE_PEAK_AMPLITUDE_TOLERANCE) {
                aTuneState = CONVERGED;
            }
        }

        // If the autotune has not already converged, terminate after 10 cycles, or if too long between peaks, or if too long between relay steps
        if (
        #ifdef AUTOTUNE_RELAY_BIAS
            ((now - lastStepTime[0]) > (unsigned long) (AUTOTUNE_MAX_WAIT_MINUTES * 60000)) ||
        #endif
        ((now - lastPeakTime[0]) > (unsigned long) (AUTOTUNE_MAX_WAIT_MINUTES * 60000)) ||
        (peakCount >= 20)
        ) {
            aTuneState = FAILED;
        }

        if (((unsigned char) aTuneState & (CONVERGED | FAILED)) == 0) {
            return false;
        }

        // Autotune algorithm has terminated, reset autotuner variables
        *output = outputStart;

        if (aTuneState == FAILED) {
            // Do not calculate gain parameters
            return true;
        }

        // Finish up by calculating tuning parameters:

        // Calculate ultimate gain
        double Ku = 4.0 * workingOstep / (inducedAmplitude * M_PI); 

        // Calculate ultimate period in seconds
        double Pu = (double) 0.5 * ((lastPeakTime[1] - lastPeakTime[3]) + (lastPeakTime[2] - lastPeakTime[4])) / 1000.0;  

        // Calculate gain parameters using tuning rules

        Kp = Ku / (double) KpDiv;
        Ti = Pu / (double) TiDiv;
        Td = TdDiv == 0 ? 0.0 : Pu / TdDiv;

        // Converged
        return true;
    }

    #ifdef AUTOTUNE_RELAY_BIAS
        double pidtune_processValueOffset(double avgStep1, double avgStep2) {   
            if (avgStep1 < 1e-10) {
                return 1.0;
            }
            if (avgStep2 < 1e-10) {
                return -1.0;
            }
            // ratio of step durations
            double r1 = avgStep1 / avgStep2;
            double s1 = (sumInputSinceLastStep[1] + sumInputSinceLastStep[3]);
            double s2 = (sumInputSinceLastStep[2] + sumInputSinceLastStep[4]);
            if (s1 < 1e-10) {
                return 1.0;
            }
            if (s2 < 1e-10) {
                return -1.0;
            }
            // ratio of integrated process values
            double r2 = s1 / s2;

            /**
             * This is the explanation of the math behind this function just in case you were wondering...
             * I have no clue as to how it works, thanks so much to t0mpr1c3 once again for this!
             * ~Myles
             * 
             * 
             * Estimate process value offset assuming a trapezoidal response curve
             * Assume trapezoidal wave with amplitude a, cycle period t, time at minimum/maximum m * t (0 <= m <= 1)
             * 
             * With no offset:
             * Area under half wave of process value given by:
             *   a * m * t/2 + a/2 * (1 - m) * t/2 = a * (1 + m) * t / 4
             * 
             * Now with offset d * a (-1 <= d <= 1): 
             * Step time of relay half-cycle given by:
             *   m * t/2 + (1 - d) * (1 - m) * t/2 = (1 - d + d * m) * t/2
             * 
             * => Ratio of step times in cycle given by:
             * (1) r1 = (1 - d + d * m) / (1 + d - d * m)
             * 
             * Area under offset half wave = a * (1 - d) * m * t/2 + a/2 * (1 - d) * (1 - d) * (1 - m) * t/2
             *                             = a * (1 - d) * (1 - d + m * (1 + d)) * t/4 
             * 
             * => Ratio of area under offset half waves given by:
             * (2) r2 = (1 - d) * (1 - d + m * (1 + d)) / ((1 + d) * (1 + d + m * (1 - d)))
             * 
             * Want to calculate d as a function of r1, r2; not interested in m
             * Rearranging (1) gives:
             * (3) m = 1 - (1 / d) * (1 - r1) / (1 + r1)
             * 
             * Substitute (3) into (2):
             * r2 = ((1 - d) * (1 - d + 1 + d - (1 + d) / d * (1 - r1) / (1 + r1)) / ((1 + d) * (1 + d + 1 - d - (1 - d) / d * (1 - r1) / (1 + r1)))   
             * 
             * After much algebra, we arrive at: 
             * (4) (r1 * r2 + 3 * r1 + 3 * r2 + 1) * d^2 - 2 * (1 + r1)(1 - r2) * d + (1 - r1) * (1 - r2) = 0
             * 
             * Quadratic solution to (4):
             * 5) d = ((1 + r1) * (1 - r2) +/- 2 * sqrt((1 - r2) * (r1^2 - r2))) / (r1 * r2 + 3 * r1 + 3 * r2 + 1)
            */

            // Estimate offset as proportion of amplitude
            double discriminant = (1.0 - r2) * (pow(r1, 2) - r2);
            // Catch negative values
            if (discriminant < 1e-10) {
                discriminant = 0.0;
            }
            // Return estimated process value offset
            return ((1.0 + r1) * (1.0 - r2) + ((r1 > 1.0) ? 1.0 : -1.0) * sqrt(discriminant)) / 
                    (r1 * r2 + 3.0 * r1 + 3.0 * r2 + 1.0);
        }
    #endif // AUTOTUNE_RELAY_BIAS

    void pidtune_setOutputStep(double step) {
        oStep = step;
    }

    void pidtune_setKpDivisor(uint8_t kPdiv) {
        KpDiv = kPdiv;
    }

    void pidtune_setTiDivisor(uint8_t tIdiv) {
        TiDiv = tIdiv;
    }

    void pidtune_setTdDivisor(uint8_t tDdiv) {
        TdDiv = tDdiv;
    }

    void pidtune_setNoiseBand(double band) {
        noiseBand = band;
    }

    void pidtune_setLookbackSec(int sec) {
        if (sec < 1) {
            sec = 1;
        }
        if (sec < 25) {
            nLookBack = sec * 4;
            sampleTime = 250;
        }
        else {
            nLookBack = 100;
            sampleTime = sec * 10;
        }
    }

    double pidtune_getKp() {
        return Kp;
    }

    double pidtune_getTi() {
        return Kp / Ti;
    }

    double pidtune_getTd() {
        return Kp * Td;
    }

#endif // PID_AUTOTUNE
