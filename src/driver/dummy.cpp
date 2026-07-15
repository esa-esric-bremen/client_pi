#include "dummy.hpp"

int last_reading = 350;
int direction = 1;

int readRandom()
{
    return rand() % 100;
}

// load cell
HX711::HX711(int dout, int pd_sck, int gain) {
    (void)dout;
    (void)pd_sck;
    (void)gain;
};

double HX711::get_weight(int times)
{
    (void) times;
    last_reading += (readRandom() % 10 - 4 + 50) * direction;
    if (direction > 0 && last_reading > 5000) {
        direction = -1;
    } else if (direction < 0 && last_reading < 1) {
        direction = 1;
    }
    return last_reading;
};

void HX711::set_reading_format(const std::string& b_format, const std::string& bi_format) 
{
    (void) b_format;
    (void) bi_format;
};

void HX711::set_reference_unit(double ref_unit) { 
    (void) ref_unit;
};

void HX711::tare(int times) { 
    (void) times;
};

void HX711::reset() { 
};

// power consumption
INA228::INA228(const uint8_t address)
{
    (void) address;
}

bool INA228::begin()
{
    return true;
}

bool INA228::setMode(uint8_t mode)
{
    (void) mode;
    return true;
}

bool INA228::setAverage(uint8_t avg)
{
    (void) avg;
    return true;
}

float INA228::getBusVoltage()
{
    return readRandom();
}

float INA228::getShuntVoltage()
{
    return 2.34;
}

float INA228::getCurrent()
{
    return 3.45;
}

float INA228::getPower()
{
    return 4.56;
}

int INA228::setMaxCurrentShunt(float maxCurrent, float shunt) {
    (void) maxCurrent;
    (void) shunt;
    return 0;
}

// the other power consumption measurement
ADS1X15::ADS1X15(const uint8_t address, std::string variant, float gain)
{
    (void) address;
    (void) variant;
    this->gain = gain;
}

double ADS1X15::getCurrent()
{
    return readRandom() / 10;
}

double ADS1X15::getVoltage()
{
    return ((float)readRandom()) + 40;
}

// raspberry pi vcgencmd
int get_throttled()
{
    return 0;
};

// ACTUATORS
// GPIO/PWM drivers
void gpio_value(bool value, int gpioPin)
{
    if (value) {
        std::cout << "Turn on GPIO pin " << gpioPin << std::endl;
    } else {
        std::cout << "Turn off GPIO pin " << gpioPin << std::endl;
    }
};

void pwm_value(int value, int gpioPin)
{
    std::cout << "Set GPIO pin " << gpioPin << " to " << value << std::endl;
};

void soft_pwm_value(int value, int gpioPin) {
    pwm_value(value, gpioPin);
};

// motor driver
void set_l298n(int en, int in1, int in2, double value, bool pwm)
{
    (void) in1;
    (void) in2;
    (void) pwm;
    std::cout << "Set " << en << " to " << value << std::endl;
};