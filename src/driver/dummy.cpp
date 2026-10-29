#include "dummy.hpp"
#include "sim_model.hpp"

// load cell
HX711::HX711(int dout, int pd_sck, int gain) {
    (void) dout;
    (void) pd_sck;
    (void) gain;
}

double HX711::get_weight(int times) {
    (void) times;
    return sim_model().weight_g();
}

void HX711::set_reading_format(const std::string& byte_format, const std::string& bit_format) {
    (void) byte_format;
    (void) bit_format;
}

void HX711::set_reference_unit(double ref_unit) {
    (void) ref_unit;
}

void HX711::tare(int times) {
    (void) times;
}

void HX711::reset() {
}

// power consumption
INA228::INA228(const uint8_t address) {
    (void) address;
}

bool INA228::begin() {
    return true;
}

bool INA228::setMode(uint8_t mode) {
    (void) mode;
    return true;
}

bool INA228::setAverage(uint8_t avg) {
    (void) avg;
    return true;
}

int INA228::setMaxCurrentShunt(float maxCurrent, float shunt) {
    (void) maxCurrent;
    (void) shunt;
    return 0;
}

float INA228::getBusVoltage() {
    return sim_model().voltage_v();
}

float INA228::getShuntVoltage() {
    // 2 mOhm shunt
    return getCurrent() * 0.002f;
}

float INA228::getCurrent() {
    return sim_model().current_a();
}

float INA228::getPower() {
    return getBusVoltage() * getCurrent();
}

ADS1X15::ADS1X15(const uint8_t address, std::string variant, float gain) {
    (void) address;
    (void) variant;
    this->gain = gain;
}

double ADS1X15::getCurrent() {
    return sim_model().current_a();
}

double ADS1X15::getVoltage() {
    return sim_model().voltage_v();
}

double read_cpu_temperature() {
    return sim_model().cpu_temp_c();
}

// raspberry pi vcgencmd
int get_throttled() {
    return sim_model().throttled();
}

// ACTUATORS
void gpio_value(bool value, int gpioPin) {
    std::cout << "GPIO pin " << gpioPin << (value ? " on" : " off") << std::endl;
    sim_model().set_pwm(gpioPin, value ? 1 : 0);
}

void pwm_value(int value, int gpioPin) {
    std::cout << "PWM pin " << gpioPin << " = " << value << std::endl;
    sim_model().set_pwm(gpioPin, value);
}

void soft_pwm_value(int value, int gpioPin) {
    pwm_value(value, gpioPin);
}

void set_l298n(int en, int in1, int in2, double value, bool pwm) {
    (void) in1;
    (void) in2;
    (void) pwm;
    sim_model().set_motor(en, value);
}
