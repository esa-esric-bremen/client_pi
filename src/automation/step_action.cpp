#include "step_action.hpp"

StepAction::StepAction(
        const std::string taskName,
        const YAML::Node& node,
        const AutomationContext& context,
        const std::map<std::string, std::shared_ptr<Action>> actions)
        : stateChange(context.stateChange) {
    this->actions = actions;
    parse(node, taskName);
    reset();
}

void StepAction::reset() {
    completed = false;
    std::shared_ptr<Action> a;
    if (started && type == ActionType::DURATION) {
        a = actions.at(command);
        a->set_value(a->start);
    } 
    started = false;
}

void StepAction::update(const std::map<std::string, double>& current_sensor_values) {
    if (completed) {
        return;
    }
    execute(current_sensor_values);
}

bool StepAction::done() {
    return completed;
}

void StepAction::execute(const std::map<std::string, double>& current_sensor_values) {
    auto now = std::chrono::steady_clock::now();
    std::shared_ptr<Action> a;
    switch (type) {
        case ActionType::SET_VALUE:
            a = actions.at(command);
            a->set_value(value);
            std::cout << "task exec: set value " << a->value << std::endl;
            completed = true;
            return;
        case ActionType::ADD_VALUE:
            a = actions.at(command);
            a->set_value(value + a->value);
            std::cout << "task exec: add value " << a->value << std::endl;
            completed = true;
            return;
        case ActionType::DURATION: {
            if (!started) {
                if (reset_counter != "" && current_sensor_values.at(reset_counter) < 1500) {
                    count = 0.0;
                }

                a = actions.at(command);
                a->set_value(value);
                std::cout << "Set counter to " << (count+0.5) << std::endl;
                a->counterAction->set_state((count+0.5));
                std::cout << "Set " << command << " to " << value << " for " << duration.count() << "ms" << std::endl;
                started = true;
                endTime = now + duration;
            }
            if (now >= endTime) {
                a = actions.at(command);
                a->set_value(a->start);
                std::cout << "Set " << command << " back to start." << std::endl;
                completed = true;
                started = false;
                if (counter != "") {
                    count++;
                    std::cout << "Run " << command << " " << counter << " times." << std::endl;

                    a->counterAction->set_state(count);
                    // TODO: reset counter via Action
                    
                }
            }
            return;
        }
        case ActionType::LOOP:
            if (reset_counter != "") {
                count = 0.0;
            }
            stateChange(nextTask, 0);
            stateChange(nextTask, 1);
            return;
        case ActionType::START_TASK:
            // reset and restart
            stateChange(nextTask, 0);
            stateChange(nextTask, 1);
            // This task is done but another one will start.
            return;
        default:
            std::cout << "Not implemented unknown task type" << std::endl;
    }
}

void StepAction::parse(
        const YAML::Node& node,
        const std::string taskName) {
    std::string type_ = node["type"].as<std::string>();
    if (node["counter"]) {
        counter = node["counter"].as<std::string>();
        count = 0.0;
    }
    if (node["reset_counter"]) {
        reset_counter = node["reset_counter"].as<std::string>();
    }
    if (type_ == "SET_VALUE") {
        type = ActionType::SET_VALUE;
        command = node["command"].as<std::string>();
        value = node["value"].as<double>();
    } else if (type_ == "ADD_VALUE") {
        type = ActionType::ADD_VALUE;
        command = node["command"].as<std::string>();
        value = node["value"].as<double>();
    } else if (type_ == "DURATION") {
        type = ActionType::DURATION;
        command = node["command"].as<std::string>();
        value = node["value"].as<double>();
        duration = std::chrono::milliseconds(
            node["duration_ms"].as<long>());
    } else if (type_ == "START_TASK") {
        type = ActionType::START_TASK;
        nextTask = node["task"].as<std::string>();
    } else if (type_ == "LOOP") {
        nextTask = taskName;
        type = ActionType::LOOP;
    } else {
        std::cout << "Unknown Action type: " << type_ << std::endl; 
        return;
    }
};