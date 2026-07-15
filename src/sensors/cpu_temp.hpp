#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include "sensor.hpp"

class CpuTemperature : public Sensor {
public:
    CpuTemperature(YAML::Node config);
    void init() override;
    bool ready(const TimePoint& now) override;
    double get_value() override;
    void update(const TimePoint& now) override;
private:
    std::string filename;
    double scale;
};