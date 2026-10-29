#include "olimexino_sensor.hpp"

#include <algorithm>
#include <stdexcept>

OlimexinoSensor::OlimexinoSensor(const YAML::Node& config, std::shared_ptr<Olimexino> board)
    : Sensor(config), board(std::move(board)) {
    if (!config["channel"]) {
        throw std::runtime_error("sensor " + id + ": missing 'channel' (telemetry name or 'link')");
    }
    channel = config["channel"].as<std::string>();
}

bool OlimexinoSensor::ready(const TimePoint& now) {
    return due(now) && (channel == LINK || board->channel(channel).has_value());
}

double OlimexinoSensor::get_value() {
    // no burst of reads after the channel was missing for a while
    next_read_time = std::max(next_read_time + read_interval, Clock::now());
    if (channel == LINK) {
        return board->alive() ? 1.0 : 0.0;
    }
    return board->channel(channel).value_or(0.0);
}
