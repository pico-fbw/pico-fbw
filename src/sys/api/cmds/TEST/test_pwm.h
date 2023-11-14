#ifndef __TEST_PWM_H
#define __TEST_PWM_H

#define MAX_PWM_DEV 2 // Maximum deviation from the set to the read PWM value in degrees

uint api_test_pwm(const char *cmd, const char *args);

#endif // __TEST_PWM_H
