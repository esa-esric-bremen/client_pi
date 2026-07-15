// Simple gpio/PWM using only one Pin
#pragma once
#ifdef SIMULATED
#include "../driver/dummy.hpp"
#else
#include "../driver/gpio.hpp"
#endif
#include "action.hpp"
#include <string>
#include <iostream>

class GpioDriver : public Action {
private:
    int gpio;
    int pin;
    int range;
    std::string mode;
    // time controled drivers
    int rampMs;
    int desiredPos;
    std::chrono::steady_clock::time_point lastTime;

    void applyValues(double val);
public:
    GpioDriver(YAML::Node config);

    void init() override;

    void reinit() override;

    bool ready() override;

    // drive to position
    void set_value(double val) override;

    // update
    void update() override;

    void stop() override;

    void reset() override;
};