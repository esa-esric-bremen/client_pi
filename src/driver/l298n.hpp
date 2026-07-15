// used by motor_driver.hpp
#pragma once
#include <wiringPi.h>
#include "gpio.hpp"
#include <cstdlib>

void init_l298n(int en, int in1, int in2, bool pwm, int range);

void set_l298n(int en, int in1, int in2, double value, bool pwm);
