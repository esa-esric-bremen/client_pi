#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <variant>
#include <set>

#include <yaml-cpp/yaml.h>
#include <robot_remote_control/Types/RobotRemoteControl.pb.h>

#include "../actions/action.hpp"
#include "step.hpp"

// task states
#define TASK_STOPPED 0
#define TASK_RUNNING 1
#define TASK_PAUSED 2


struct ScheduledEvent {
    std::chrono::steady_clock::time_point execution_time;
    StepAction action;
};

class Task {
public:
    Task(
        YAML::Node node,
        std::map<std::string, std::shared_ptr<Action>> actions,
        const AutomationContext& context);
    std::string name;
    std::vector<Step> steps;

    // called every time we run the main loop
    void update(
        const std::map<std::string, double>& current_sensor_values);
    // start task on button press
    void start();
    void reset();
    void pause();
    void parseSteps(const YAML::Node& node);
    bool processEventQueue();
    bool checkCondition(
        const Condition& cond,
        const std::map<std::string, double>& current_sensor_values);
    void set_simple_action(robot_remote_control::SimpleAction* simpleAction) {
        this->simpleAction = simpleAction;
    };
    void set_state(int value);
private:
    std::chrono::steady_clock::time_point start_time;
    int state_value = TASK_STOPPED;

    std::map<std::string, std::shared_ptr<Action>> actions;

    // loading yaml
    robot_remote_control::SimpleAction* simpleAction;
};
