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
 * Huge thanks to 'GitJer' on GitHub for giving me a starting point for the PWM input code!
 * Check them out here: https://github.com/GitJer/Some_RPI-Pico_stuff/tree/main/PwmIn/PwmIn_4pins
*/


#ifndef pwm_h
#define pwm_h

/**
 * Enables PWM input functionality on the specified pins.
 * Up to four can be used; this is for one PIO machine (pio0).
 * @param pin_list the list of pins to enable PWM input on
 * @param num_pins the number of pins you are enabling PWM input on (likely just the size of the list)
*/
void pwm_enable(uint *pin_list, uint num_pins);

/**
 * @param readings pointer to the array where the method will store its data.
 * @param pin the number of the pin to read from the earlier specified pin_list
 * @return all PWM values from the PIO machine on the specified pin.
*/
void pwm_read(float *readings, uint pin);

/**
 * @param pin the number of the pin to read from the earlier specified pin_list
 * @return the pulsewidth of the specified pin.
*/
float pwm_readPW(uint pin);

/**
 * @param pin the number of the pin to read from the earlier specified pin_list
 * @return the duty cycle of the specified pin.
*/
float pwm_readDC(uint pin);

/**
 * @param pin the number of the pin to read from the earlier specified pin_list
 * @return the period of the specified pin.
*/
float pwm_readP(uint pin);

/**
 * @param pin the number of the pin to read from the earlier specified pin_list
 * @return the calculated degree value derived from the pulsewidth on that pin.
*/
float pwm_readDeg(uint pin);

/**
 * Samples all initalized pins for deviation from a specified value for a specified number of samples, then saves that offset value to flash.
 * @param deviation the value we should be seeing on the pin
 * @param num_samples the number of times to sample the pin for deviation
 * @param sample_delay_ms the delay between samples
 * @param run_times the amount of times to run a sampling function (num_samples), will be averaged at the end
*/
void pwm_calibrate(float deviation, uint num_samples, uint sample_delay_ms, uint run_times);

/**
 * Checks if the PWM calibration has been run before.
 * @return true if calibration has been run previously, false if not.
*/
bool pwm_checkCalibration();

/**
 * @param pin the pin (0-3 to get the value of)
 * @return the calibration value from PWM calibration.
 * Be aware that this value may not be cohesive; this function does not check to see whether or not a calibration has been done, so it is able to return random data.
*/
float pwm_getCalibrationValue(uint pin);

#endif // pwm_h
