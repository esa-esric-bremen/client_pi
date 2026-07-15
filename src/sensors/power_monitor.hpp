// use Ina228 driver
#pragma once
#ifdef SIMULATED
#include "../driver/dummy.hpp"
#else
#include "../driver/ina228.hpp"
#include "../driver/ads1x15.hpp"
#endif
#include "sensor.hpp"

class PowerMonitor : public Sensor {
public:
    PowerMonitor(YAML::Node config, std::string driver);
    void init() override;
    bool ready(const TimePoint& now) override;
    double get_value() override;
    void update(const TimePoint& now) override;
private:
    int addr;
    float max_current;
    float r_shunt;
    std::string reading;
    std::string variant;
};