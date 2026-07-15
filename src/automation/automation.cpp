#include "automation.hpp"
#include "automation_context.hpp"
#include <iostream>
#include <fstream>
#include <cmath>
#include <yaml-cpp/yaml.h>

Automation::Automation(
        YAML::Node config,
        std::map<std::string, std::shared_ptr<Action>> actions) {
    load(config, actions);
}

void Automation::load(
        YAML::Node config,
        std::map<std::string, std::shared_ptr<Action>> actions) {
    if (!config["tasks"]) {
        return;
    }

    tasks.clear();
    for (const auto& task_node : config["tasks"]) {
        AutomationContext context;
        context.stateChange = [this](const std::string& task, int status) {
            this->state(task, status);
        };
        auto task = std::make_shared<Task>(
            task_node, actions, context);
        tasks[task->name] = task;
    }
    std::cout << "Successfully loaded " << tasks.size() << " tasks." << std::endl;
}

void Automation::state(const std::string& name, int new_state) {
    if (tasks.find(name) == tasks.end()) {
        std::cout << "Can not change state, task " << name << " does not exist!" << std::endl;
        return;
    }
    std::shared_ptr<Task>& task = tasks[name];
    if (new_state == 1) {
        task->start();
        return;
    }
    if (new_state == 2) {
        task->pause();
        return;
    }
    task->reset();
}

void Automation::update(
        const std::map<std::string, double>& current_sensor_values) {
    for (auto& task_pair : tasks) {
        task_pair.second->update(current_sensor_values);
    }
}