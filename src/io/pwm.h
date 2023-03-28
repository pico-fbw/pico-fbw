#ifndef pwm_h
#define pwm_h

#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"

int pwmEnable(uint *pin_list, uint num_pins);


void readPWM(float *readings, uint pin);


float readPW(uint pin);


float readD(uint pin);


float readP(uint pin);


static void pio_irq_handler();

#endif