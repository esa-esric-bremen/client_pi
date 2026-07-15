#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <variant>
#include <set>

#include <yaml-cpp/yaml.h>

#include "task.hpp"
#include "../actions/action.hpp"

class Automation : public std::enable_shared_from_this<Automation> {
public:
    Automation(
        YAML::Node config,
        std::map<std::string, std::shared_ptr<Action>> actions);
    void load(
        YAML::Node config,
        std::map<std::string, std::shared_ptr<Action>> actions);
    void state(const std::string& name, int new_state);
    void update(
        const std::map<std::string, double>& current_sensor_values);

    std::map<std::string, std::shared_ptr<Task>> tasks;
};
