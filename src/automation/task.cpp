#include "task.hpp"
#include "automation.hpp"
#include "automation_context.hpp"
#include <iostream>
#include <fstream>
#include <cmath>
#include <yaml-cpp/yaml.h>

Task::Task(
        YAML::Node node,
        std::map<std::string, std::shared_ptr<Action>> actions,
        const AutomationContext& context) {
    name = node["name"].as<std::string>();
    for (const auto& stepNode : node["steps"]) {
        Step s(name, stepNode, context, actions);
        steps.push_back(s);
    }
}

void Task::set_state(int value) {
    simpleAction->set_state(value);
    state_value = value;
}

void Task::reset() {
    // empty step queue
    std::cout << "Reset task " << name << std::endl;
    // initiate/reset states
    auto now = std::chrono::steady_clock::now();
    // continue or pause?
    set_state(TASK_STOPPED);
    start_time = now;
    for (auto& step : steps) {
        step.reset();
    }
}

void Task::pause() {
    set_state(TASK_PAUSED);
    std::cout << "Pause task " << name << std::endl;
}

void Task::update(
        const std::map<std::string, double>& current_sensor_values
    ) {
    (void) current_sensor_values;
    if (state_value == TASK_STOPPED || state_value == TASK_PAUSED) {
        return;
    }

    bool allDone = true;
    for (auto& step : steps) {
        // steps can run in parallel so we call all steps during one update.
        step.update(current_sensor_values);
        if (!step.done()) {
            allDone = false;
        }
    }
    if (allDone) {
        set_state(TASK_STOPPED);
        std::cout << "Task " << name << " done" << std::endl;
        for (auto& step : steps) {
            step.reset();
        }
    }
}



void Task::start() {
    std::cout << "Start task " << name << std::endl;
    // initiate/reset states
    auto now = std::chrono::steady_clock::now();
    // continue or pause?
    start_time = now;
    // this lets the update loop execute the steps
    set_state(TASK_RUNNING);
}

