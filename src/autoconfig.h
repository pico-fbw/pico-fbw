#ifndef __AUTOCONFIG_H
#define __AUTOCONFIG_H

/* This file, as the name suggests, automatically completes some configuration steps left over from config.h, such as merging different sections. */

#include "config.h"

// TODO: implement this
#if defined(CONTROL_3AXIS)
	#define INPUT0 INPUT_AIL_PIN
	#define SERVO0 SERVO_AIL_PIN
	#define INPUT1 INPUT_ELEV_PIN
	#define OUTPUT1 SERVO_ELEV_PIN
	#define INPUT2 INPUT_RUD_PIN
	#define OUTPUT2 SERVO_RUD_PIN
    
    #define CTRL_NUM_IN 3
    #define CTRL_NUM_OUT 3
#elif defined(CONTROL_FLYINGWING)
	#define INPUT0 INPUT_ELEVON_L_PIN
	#define OUTPUT0 SERVO_ELEVON_L_PIN
	#define INPUT1 INPUT_ELEVON_R_PIN
	#define OUTPUT1 SERVO_ELEVON_R_PIN
    #undef INPUT2
    #undef OUTPUT2

    #define CTRL_NUM_IN 2
    #define CTRL_NUM_OUT 2
#endif

#ifdef ATHR_ENABLED
    #define NUM_INPUT_PINS CTRL_NUM_IN + 2 // Mode switch pin + athr pin
#else
    #define NUM_INPUT_PINS CTRL_NUM_IN + 1
#endif
#define NUM_OUTPUT_PINS CTRL_NUM_OUT

#endif // __AUTOCONFIG_H