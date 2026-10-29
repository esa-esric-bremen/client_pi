// telemetry channel for the OLIMEXINO as a sensor.
#pragma once

#include <memory>

#include "sensor.hpp"
#include "../driver/olimexino.hpp"

class OlimexinoSensor : public Sensor {
public:
    OlimexinoSensor(const YAML::Node& config, std::shared_ptr<Olimexino> board);
    void init() override {}
    double get_value() override;
    void update(const TimePoint& /*now*/) override {}
    // missing channels are not reported, the UI shows them as outdated
    bool ready(const TimePoint& now) override;

private:
    static constexpr const char* LINK = "link";
    std::shared_ptr<Olimexino> board;
    std::string channel;
};
