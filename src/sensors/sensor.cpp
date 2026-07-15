#include "sensor.hpp"

Sensor::Sensor(YAML::Node config) {
    this->name = config["name"].as<std::string>();
    this->id = config["id"].as<std::string>();
    this->driver = config["driver"].as<std::string>();
    this->mask = config["mask"].as<int>();
    this->frequency = config["frequency"].as<double>();
}

bool Sensor::due(const TimePoint& now) {
    return now >= this->next_read_time;
}

void Sensor::init_interval(const TimePoint& start_time) {
    if (this->frequency <= 0) {
        this->frequency = 1;
    }
    // first read time is now
    this->next_read_time = start_time;
    double interval_seconds = 1.0 / this->frequency;
    this->read_interval = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::duration<double>(interval_seconds)
    );
}