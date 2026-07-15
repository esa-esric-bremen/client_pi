#pragma once
#include <thread>
#include <iostream>
#include <mutex>
#include <atomic>
#include <chrono>
#ifdef SIMULATED
#include "../driver/dummy.hpp"
#else
#include "../driver/hx711.hpp"
#endif
#include <yaml-cpp/yaml.h>
#include "sensor.hpp"

class WeightSensor : public Sensor {
public:
    WeightSensor(YAML::Node config);
    ~WeightSensor();

    void init() override;
    void threaded_init();
    bool ready(const TimePoint& now) override;
    double get_value() override;
    void update(const TimePoint& now) override;
private:
    std::unique_ptr<HX711> hx711_sensor;
    void threaded_update();

    double calibration_weight = 0;
    int runs_start_calibrate = 0;
    int runs_calibrate = 5;
    int runs_init = 0;
    double weight;
    // from YAML
    int gpioPinDT;
    int gpioPinSCK;
    int zero_value;
    int known_weight;
    int weight_value;

    std::thread update_thread_;
    std::mutex data_mutex_;
    std::atomic<bool> init_done_{false};
    std::atomic<bool> is_running_{false};
};