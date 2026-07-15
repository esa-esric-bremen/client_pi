#pragma once
// a step executes a list of actions to control effectors on the robot
// on the 
#include <vector>
#include <chrono>
#include <variant>
#include <iostream>
#include <set>
#include <string>
#include <yaml-cpp/yaml.h>
#include "automation_context.hpp"
#include "step_action.hpp"
#include "../actions/action.hpp"

// forward declaration so we can access other
// tasks and execute them
class Automation;

// Condition-related structs remain the same
enum class ComparisonOp { GREATER_THAN, LESS_THAN, EQUALS };
enum class ConditionType { 
    // just execute now
    IMMEDIATE,
    // time since we started the task
    TIME_ELAPSED_FROM_START,
    // overall sensor value
    SENSOR_VALUE,
    // difference since last time we measured
    SENSOR_DIFF
};

struct Condition {
    ConditionType type;
    std::variant<std::monostate, std::chrono::milliseconds, std::string, int, double> param1;
    std::variant<std::monostate, ComparisonOp, double> param2;
    std::variant<std::monostate, std::chrono::milliseconds, double> param3;
};

class Step {
public:
    Step(
        const std::string taskName,
        const YAML::Node& node,
        const AutomationContext& context,
        const std::map<std::string, std::shared_ptr<Action>> actions);
    bool done();
    void update(const std::map<std::string, double>& current_sensor_values);
    void reset();
    bool executeAction(
        const StepAction& StepAction);
private:
    std::chrono::steady_clock::time_point start_time;

    bool started;
    bool completed;

    std::map<std::string, double> last_sensor_diff;

    int id;
    Condition condition;
    std::vector<StepAction> stepActions;

    // loading yaml
    Condition parseCondition(const YAML::Node& node);
    std::vector<StepAction> parseActions(
        const AutomationContext& context,
        const std::string taskName,
        const YAML::Node& node,
        const std::map<std::string, std::shared_ptr<Action>> actions);

    bool checkCondition(
        const std::map<std::string, double>& current_sensor_values);
};