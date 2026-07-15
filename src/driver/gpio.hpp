#pragma once
#include <wiringPi.h>
#include <softPwm.h>

void init_gpio(int gpioPin);
void init_pwm(int gpioPin, int range, int start);
void init_soft_pwm(int gpioPin, int range, int start);

void gpio_value(bool value, int gpioPin);
void pwm_value(int value, int gpioPin);
void soft_pwm_value(int value, int gpioPin);
