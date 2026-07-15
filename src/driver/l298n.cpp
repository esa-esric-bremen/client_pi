#include "l298n.hpp"
#include <iostream>
#include <wiringPi.h>

void init_l298n(
  int en, int in1, int in2, bool pwm, int range)
{
  if (pwm) {
    pinMode(en, PWM_OUTPUT);
    pwmSetMode(PWM_MODE_MS);
    pwmSetRange(range);
    pwmSetClock(192);
  } else {
    pinMode(en, OUTPUT);
  }
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
}


void set_l298n(int en, int in1, int in2, double value, bool pwm)
{
  digitalWrite(in1, value < 0 ? HIGH : LOW);
  digitalWrite(in2, value > 0 ? HIGH : LOW);

  if (pwm) {
    pwmWrite(en, std::abs(value));
  } else {
    digitalWrite(en, std::abs(value) == 0 ? LOW : HIGH);
  }
}
