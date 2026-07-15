#pragma once
#include <string>
#include <chrono>
#include <yaml-cpp/yaml.h>

using namespace std::chrono_literals;

class Sensor {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    std::string name;
    std::string id;
    int mask;
    std::string driver;
    double frequency;

    Sensor(YAML::Node config);
    bool due(const TimePoint& now);
    void init_interval(const TimePoint& start_time);

    virtual ~Sensor() {}
    virtual void init() = 0;
    virtual double get_value() = 0;
    virtual void update(const TimePoint& now) = 0;
    virtual bool ready(const TimePoint& now) = 0;
protected:
    TimePoint next_read_time;
    std::chrono::milliseconds read_interval;
};