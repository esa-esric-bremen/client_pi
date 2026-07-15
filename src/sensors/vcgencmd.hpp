#pragma once
#include <iostream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <algorithm>
#include <yaml-cpp/yaml.h>
#include "sensor.hpp"

class Vcgencmd : public Sensor {
public:
    Vcgencmd(YAML::Node config);
    void init() override;
    bool ready(const TimePoint& now) override;
    double get_value() override;
    void update(const TimePoint& now) override;
private:
};

std::string exec(const char* cmd);