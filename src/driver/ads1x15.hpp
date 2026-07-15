#pragma once
#include <string>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <ads1115.h>

// Define a base pin number for the ADS1115.
#define BASE 120


class ADS1X15
{
public:
    ADS1X15(const uint8_t address, std::string variant, float gain);
    double getVoltage();
    double getCurrent();
private:
    std::string variant;
    float gain;
    double convertToVoltage(int raw_value);
};