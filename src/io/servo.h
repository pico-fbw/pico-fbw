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
 * Huge thanks to 'markushi' on GitHub for developing the bulk of this servo library! (slightly modified by MylesAndMore)
 * Check them out here: https://github.com/markushi/pico-servo
*/

#ifndef servo_h
#define servo_h

/**
 * Set the frequency, making "top" as large as possible for maximum resolution.
 * Maximum "top" is set at 65534 to be able to achieve 100% duty with 65535.
*/
#define SERVO_TOP_MAX 65534

/**
 * Enable Servo control.
 * @param gpio_pin: the GPIO pin the servo is attached to
 * 
 * @return 0 if the operation was successful
*/
int servo_enable(const uint gpio_pin);

/**
 * Enable Servo control.
 * @param gpio_pin: the GPIO pin the servo is attached to
 * 
 * @return 0 if the operation was successful
*/
int servo_disable(const uint gpio_pin);

/**
 * Sets the position of the servo using the the duty cycle of the PWM signal.
 *
 * @param gpio_pin: the GPIO pin the servo is attached to
 * @param degree: the position in degree, a value within 0-180
 *
 * @return 0 if the operation was successful
*/
int servo_set(const uint gpio_pin, const uint16_t degree);

#endif // servo_h