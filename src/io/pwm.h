#ifndef __PWM_H
#define __PWM_H

/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Markus Hintersteiner
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * **/

/**
 * Huge thanks to 'GitJer' on GitHub for most of the PWM input code!
 * Check them out here: https://github.com/GitJer/Some_RPI-Pico_stuff/tree/main/PwmIn/PwmIn_4pins
*/

/**
 * Enables PWM input functionality on the specified pins.
 * Up to eight pins can be enabled; over four will use PIO1 in addition to PIO0.
 * @param pin_list the list of pins to enable PWM input on
 * @param num_pins the number of pins you are enabling PWM input on (1-8)
*/
void pwm_enable(uint *pin_list, uint num_pins);

/**
 * @param pin the number of the pin to read (0-7)
 * @return the pulsewidth of the specified pin.
*/
float pwm_readPulseWidth(uint pin);

/**
 * @param pin the number of the pin to read (0-7)
 * @return the duty cycle of the specified pin.
*/
float pwm_readDutyCycle(uint pin);

/**
 * @param pin the number of the pin to read (0-7)
 * @return the period of the specified pin.
*/
float pwm_readPeriod(uint pin);

/**
 * @param pin the number of the pin to read (0-7)
 * @return the calculated degree value derived from the pulsewidth on that pin.
*/
float pwm_readDeg(uint pin);

/**
 * Samples all initalized pins for deviation from a specified value for a specified number of samples, then saves that offset value to flash.
 * @param deviations the value we should be seeing on each pin
 * @param num_samples the number of times to sample the pin for deviation
 * @param sample_delay_ms the delay between samples
 * @param run_times the amount of times to run a sampling function (num_samples), will be averaged at the end
 * 
 * @return true if the calibration was successful, false if not
*/
bool pwm_calibrate(float deviations[], uint num_samples, uint sample_delay_ms, uint run_times);

/**
 * Checks if the PWM calibration has been run before.
 * @return 0 if calibration has been run previously, -1 if no calibration has been run, and -2 if calibration values seem abnormal.
*/
int pwm_isCalibrated();

/**
 * @return true if the value is within the maximum calibration offset.
*/
#define WITHIN_MAX_CALIBRATION_OFFSET(value) ((value) >= -MAX_CALIBRATION_OFFSET && (value) <= MAX_CALIBRATION_OFFSET)

#endif // __PWM_H
