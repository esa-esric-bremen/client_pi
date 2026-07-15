#include "gpio_driver.hpp"
#include <string>

GpioDriver::GpioDriver(YAML::Node config) {
    this->name = config["name"].as<std::string>();
    this->command = config["command"].as<std::string>();
    this->driver = config["driver"].as<std::string>();
    this->mode = config["mode"].as<std::string>();
    this->pin = config["pin"].as<int>();
    this->start = config["start"].as<int>();
    this->min = config["range"].as<std::vector<int>>()[0];
    this->max = config["range"].as<std::vector<int>>()[1];

    if (this->mode == "pwm" || this->mode == "soft_pwm") {
        this->range = config["pwm_range"].as<int>();
    }
    if (config["ramp_ms"]) {
        this->rampMs = config["ramp_ms"].as<int>();
    } else {
        this->rampMs = 0;
    }
    if (config["counter"]) {
        this->counter = config["counter"].as<std::string>();
    }
    this->desiredPos = this->start;
    this->value = this->start;
}

void GpioDriver::init() {
    std::cout << "  using pin " << this->pin << "for gpio" << std::endl;
#ifndef SIMULATED
    if (this->mode == "pwm") {
        init_pwm(this->pin, this->range, this->start);
    } else if (this->mode == "gpio") {
        init_gpio(this->pin);
    } else if (this->mode == "soft_pwm") {
        init_soft_pwm(this->pin, this->range, this->start);
    }
#endif
}

void GpioDriver::reinit() {
    init();
}

bool GpioDriver::ready() {
    return true;
}

void GpioDriver::stop() {
    set_value(this->start);
}

void GpioDriver::reset() {
    stop();
}

void GpioDriver::applyValues(double val) {
    this->value = val;
    simpleAction->set_state(val);
    if (this->mode == "pwm") {
        pwm_value(this->value, pin);
    } else if (this->mode == "soft_pwm") {
        soft_pwm_value(this->value, pin);
    } else if (this->mode == "gpio") {
        gpio_value(this->value > 0, pin);
    }
}

void GpioDriver::set_value(double val) {
    if (this->rampMs > 0) {
        std::cout << "new desired position:" << val << std::endl;
        this->desiredPos = val;
        return;
    }
    applyValues(val);
}

void GpioDriver::update() {
    if (this->rampMs > 0 && this->value != this->desiredPos) {
        auto now = std::chrono::steady_clock::now();
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
        auto lastTime_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            lastTime.time_since_epoch()).count();
        auto elapsedTime = now_ms - lastTime_ms;
        if (elapsedTime < rampMs) {
            return;
        }
        if (this->value > this->desiredPos) {
            lastTime = now;
            applyValues(this->value-1);
        }
        else if (this->value < this->desiredPos) {
            lastTime = now;
            applyValues(this->value+1);
        }
    }
}