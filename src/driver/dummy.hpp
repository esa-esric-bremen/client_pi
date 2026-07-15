// Dummy driver to provide random numbers for testing
#pragma once
#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <cstdint>
#include "ina288_enums.h"

int readRandom();

// SENSORS
// load cells
class HX711 {
public:
    explicit HX711(int dout, int pd_sck, int gain = 128);
    double get_weight(int times = 3);
    void set_reading_format(const std::string& byte_format = "MSB", const std::string& bit_format = "MSB");
    void set_reference_unit(double ref_unit);
    void tare(int times = 15);
    void reset();
};

// power consumption
class INA228
{
public:
  explicit INA228(const uint8_t address);
  bool begin();
  bool setMode(uint8_t mode = INA228_MODE_CONT_TEMP_BUS_SHUNT);
  bool setAverage(uint8_t avg = INA228_1_SAMPLE);
  int setMaxCurrentShunt(float maxCurrent, float shunt);
  float getBusVoltage();
  float getShuntVoltage();
  float getCurrent();
  float getPower();
};

class ADS1X15
{
private:
  float gain;
public:
  explicit ADS1X15(const uint8_t address, std::string variant, float gain);
  double getVoltage();
  double getCurrent();
};
// for temperature we use the real temperature sensor of the CPU

// raspberry pi vcgencmd
int get_throttled();

// ACTUATORS
// GPIO/PWM drivers
void gpio_value(bool value, int gpioPin);
void pwm_value(int value, int gpioPin);
void soft_pwm_value(int value, int gpioPin);

// motor driver
void set_l298n(int en, int in1, int in2, double value, bool pwm);

