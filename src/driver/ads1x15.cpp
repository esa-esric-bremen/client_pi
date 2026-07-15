#include "ads1x15.hpp"
#include <iostream>

ADS1X15::ADS1X15(
    const uint8_t address, std::string variant, float gain)
{
    (void) address;
    this->variant = variant;
    this->gain = gain;
    ads1115Setup(BASE, address);
}

double ADS1X15::convertToVoltage(int raw_value) {
    int bits = 16; // ads1115
    if (this->variant == "ads1015") {
        bits = 12;
    }
    double lsb = this->gain / (1 << (bits - 1));
    raw_value >>= 16 - bits;
    double volts = raw_value * lsb;

    return volts;
}

double ADS1X15::getVoltage() {
    int raw = analogRead(BASE + 3);
    double voltage = convertToVoltage(raw) * 13;
    // std::cout << "raw voltage: " << convertToVoltage(raw) << std::endl;
    return voltage;
}

double ADS1X15::getCurrent() {
    int raw = analogRead(BASE + 0);
    double voltage = convertToVoltage(raw);
    double amps = (voltage-2.5) * 13.88888888888889;
    // std::cout << "raw voltage for amps: " << voltage << std::endl;
    return amps;
}