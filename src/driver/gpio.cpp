#include "gpio.hpp"

void init_pwm(int gpioPin, int range, int start)
{
    pinMode(gpioPin, PWM_OUTPUT);
    pwmSetMode(PWM_MODE_MS);
    pwmSetClock(192);
    pwmSetRange(range);  // should be 2000 for default
    pwmWrite(gpioPin, start);  // should be 150 for DFKI motors
}

void init_soft_pwm(int gpioPin, int range, int start)
{
    pinMode(gpioPin, OUTPUT);
    softPwmCreate(gpioPin, start, range);
}

void init_gpio(int gpioPin)
{
    //wiringPiSetupGpio(); // Initialize wiringPi to use GPIO pin numbers (not WiringPi numbers)
    pinMode(gpioPin, OUTPUT);
}

void gpio_value(bool value, int gpioPin)
{
    digitalWrite(gpioPin, value == false ? 0 : 1);
}

void pwm_value(int value, int gpioPin)
{
    pwmWrite(gpioPin, value);
}

void soft_pwm_value(int value, int gpioPin)
{
    softPwmWrite(gpioPin, value);
}