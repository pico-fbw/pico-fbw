#ifndef __PIDTUNE_H
#define __PIDTUNE_H

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

#include "../config.h"

#ifdef PID_AUTOTUNE

    enum Peak {
        MINIMUM = -1,
        NOT_A_PEAK = 0,
        MAXIMUM = 1
    };

    enum AutoTunerState {
        AUTOTUNER_OFF = 0, 
        STEADY_STATE_AT_BASELINE = 1,
        STEADY_STATE_AFTER_STEP_UP = 2,
        RELAY_STEP_UP = 4,
        RELAY_STEP_DOWN = 8,
        CONVERGED = 16,
        FAILED = 128
    }; 

    // Tuning rule divisors
    enum {
        KP_DIVISOR = 0,
        TI_DIVISOR = 1,
        TD_DIVISOR = 2
    };


    /**
     * Initializes the PID tuning process.
     * @param Input pointer to input value
     * @param Output pointer to output value
    */
    void pidtune_init(double* Input, double* Output);

    /**
     * Cancels the PID tuning process.
    */
    void pidtune_cancel();

    /**
     * Runs one cycle of the PID tuning process.
     * @return false if the calibration is currently running, true otherwise.
    */
    bool pidtune_runtime();

    /**
     * Calculates offset of oscillation in process value as a proportion of the amplitude approximation.
     * Assumes a trapezoidal oscillation that is stationary over the last 2 relay cycles. 
     * Needs constant phase lag, so recent changes to noiseBand are bad.
    */
    double pidtune_processValueOffset(double avgStep1, double avgStep2);

    /**
     * Sets the output step size.
    */
    void pidtune_setOutputStep(double step);

    /**
     * Sets the PID tuning rule, Kp divisor.
    */
    void pidtune_setKpDivisor(uint8_t kPdiv);

    /**
     * Sets the PID tuning rule, Ti divisor.
    */
    void pidtune_setTiDivisor(uint8_t tIdiv);

    /**
     * Sets the PID tuning rule, Td divisor.
    */
    void pidtune_setTdDivisor(uint8_t tDdiv);

    /**
     * Sets the PID noise band.
    */
    void pidtune_setNoiseBand(double band);

    /**
     * Sets the PID tuning lookback time in seconds.
    */
    void pidtune_setLookbackSec(int sec);

    /**
     * @return the final PID tuning rule, Kp.
    */
    double pidtune_getKp();

    /**
     * @return the final PID tuning rule, Ti.
    */
    double pidtune_getTi();

    /**
     * @return the final PID tuning rule, Td.
    */
    double pidtune_getTd();

#endif // PID_AUTOTUNE

#endif // __PIDTUNE_H
