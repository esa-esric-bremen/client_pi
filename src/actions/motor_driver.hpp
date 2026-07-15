// control for motor driver like l298n
#pragma once
#ifdef SIMULATED
#include "../driver/dummy.hpp"
#else
#include "../driver/l298n.hpp"
#endif
#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>
#include <string>
#include "action.hpp"

using namespace std::chrono_literals;

class MotorDriver : public Action {
private:
    int en;
    int in1;
    int in2;
    bool pwm;
    std::string mode;

    // direction the motor moves
    int direction;

    // the relative position we want the motor to be
    long long desiredTime;
    // the time we moved the motor so far (distance relative to 0)
    long long positionTime;
    
    std::string control;

    std::chrono::steady_clock::time_point lastTime;
    std::chrono::duration<double> targetDuration;

public:
    MotorDriver(YAML::Node config);

    void init() override;

    void reinit() override;

    void stop() override;

    bool ready() override;

    // drive to position
    void set_value(double val) override;

    // update
    void update() override;

    void reset() override;
};