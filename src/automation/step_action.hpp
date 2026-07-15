#pragma once
#include <functional>
#include <string>
#include <memory>
#include <ctime>
#include <iomanip>
#include "../actions/action.hpp"
#include "automation_context.hpp"

// atomic description of an action we want to execute from the step
enum class ActionType {
    // runs an action for a duration (excavator or screw)
    DURATION,
    // set to a fixed value, e.g. linear_actuator
    SET_VALUE,
    // add to previous value since start
    ADD_VALUE,
    START_TASK,
    // restart (with break condition)
    LOOP
};

class StepAction {
public:
    using StateChangeCallback = std::function<void(const std::string&, int)>;

    StepAction(
        const std::string taskName,
        const YAML::Node& node,
        const AutomationContext& context,
        const std::map<std::string, std::shared_ptr<Action>> actions
    );
    // returns True if the action has been executed.
    bool done();
    // called from main loop to execute actions and check if we are complete
    void update(const std::map<std::string, double>& current_sensor_values);
    void reset();
    void execute(const std::map<std::string, double>& current_sensor_values);
    void parse(
        const YAML::Node& node,
        const std::string taskName
    );
private:
    StateChangeCallback stateChange;
    std::map<std::string, std::shared_ptr<Action>> actions;

    bool completed;

    // time we have started, only important for duration
    bool started = false;
    std::chrono::steady_clock::time_point endTime;

    ActionType type;
    std::string command;
    double value = 0.0;
    // only if type is start_task or loop
    std::string nextTask = "";
    // optional - only used by DURATION,
    // sets value back to 0.0 after run.
    std::chrono::milliseconds duration{0};
    std::string counter = "";
    std::string reset_counter = "";
    double count = 0.0;
};