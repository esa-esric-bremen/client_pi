#include "step.hpp"

Step::Step(
        const std::string taskName,
        const YAML::Node& node,
        const AutomationContext& context,
        const std::map<std::string, std::shared_ptr<Action>> actions) {
    this->id = node["id"].as<int>();
    this->condition = parseCondition(node["condition"]);
    // parse step_actions
    this->stepActions = parseActions(context, taskName, node["actions"], actions);
    completed = false;
    last_sensor_diff.clear();
}

void Step::reset() {
    last_sensor_diff.clear();
    auto now = std::chrono::steady_clock::now();
    start_time = now;
    completed = false;
    for (auto& stepAction : stepActions) {
        stepAction.reset();
    }
}

bool Step::done() {
    return completed;
}

void Step::update(const std::map<std::string, double>& current_sensor_values) {
    // executes stepactions if ready in order
    // if our step matches the condition we run all step_actions in order
    if (!completed && this->checkCondition(current_sensor_values)) {
        std::cout << " run step " << this->id << std::endl;
        // run each step IN SEQUENCE
        // so if one is not ready we just return
        bool allDone = true;
        for (auto& stepAction : stepActions) {
            stepAction.update(current_sensor_values);
            if (!stepAction.done()) {
                allDone = false;
                // do not execute future tasks yet!
                break;
            }
        }
        completed = allDone;
        if (completed) {
            std::cout << " step done: " << this->id << std::endl;
        }
    }
    // TODO: call checkCondition
    // TODO: for each step_action check completed and set overall completed
}

bool Step::checkCondition(
        const std::map<std::string, double>& current_sensor_values) {
    auto now = std::chrono::steady_clock::now();
    switch (condition.type) {
        case ConditionType::IMMEDIATE: return true;
        case ConditionType::TIME_ELAPSED_FROM_START:
            return (now - start_time) >= std::get<std::chrono::milliseconds>(condition.param1);
        case ConditionType::SENSOR_VALUE: {
            const auto& sensor_id = std::get<std::string>(condition.param1);
            if (current_sensor_values.find(sensor_id) == current_sensor_values.end()) return false;
            double value = current_sensor_values.at(sensor_id);
            auto op = std::get<ComparisonOp>(condition.param2);
            double threshold = std::get<double>(condition.param3);
            if (op == ComparisonOp::GREATER_THAN) return value > threshold;
            if (op == ComparisonOp::LESS_THAN) return value < threshold;
            if (op == ComparisonOp::EQUALS) return value == threshold;
            return false;
        }
        case ConditionType::SENSOR_DIFF: {
            const auto& sensor_id = std::get<std::string>(condition.param1);
            if (current_sensor_values.find(sensor_id) == current_sensor_values.end()) return false;
            double value = current_sensor_values.at(sensor_id);
            auto op = std::get<ComparisonOp>(condition.param2);
            double change = std::get<double>(condition.param3);
            if (last_sensor_diff.find(sensor_id) == last_sensor_diff.end()) {
                last_sensor_diff[sensor_id] = value;
                return false;
            }
            double last_diff = last_sensor_diff.at(sensor_id);
            bool cond_result = false;
            std::cout << "current: " << value << " last: " << last_diff << std::endl;
            if (op == ComparisonOp::GREATER_THAN) {
                cond_result = value > last_diff + change;
                if (value < last_diff) {
                    // make sure last_diff is always bigger then value
                    last_sensor_diff[sensor_id] = value;
                }
            } else if (op == ComparisonOp::LESS_THAN) {
                cond_result = value < last_diff + change;
            } else if (op == ComparisonOp::EQUALS) {
                cond_result = value == last_diff + change;
            }

            if (cond_result && completed) {
                last_sensor_diff[sensor_id] = value;
                std::cout << " new sensor value: " << value << std::endl;
            }
            return cond_result;
        }
    }
    return false;
}

// --- YAML parsing ---
ComparisonOp stringToOp(const std::string& str) {
    if (str == "GREATER_THAN") return ComparisonOp::GREATER_THAN;
    if (str == "LESS_THAN") return ComparisonOp::LESS_THAN;
    if (str == "EQUALS") return ComparisonOp::EQUALS;
    throw std::runtime_error("Unknown comparison operator: " + str);
}


Condition Step::parseCondition(const YAML::Node& node) {
    Condition cond;
    std::string type = node["type"].as<std::string>();
    if (type == "IMMEDIATE") {
        cond.type = ConditionType::IMMEDIATE;
    } else if (type == "TIME_ELAPSED_FROM_START") {
        cond.type = ConditionType::TIME_ELAPSED_FROM_START;
        cond.param1 = std::chrono::milliseconds(
            node["duration_ms"].as<long>());
    } else if (type == "SENSOR_VALUE") {
        cond.type = ConditionType::SENSOR_VALUE;
        cond.param1 = node["sensor_id"].as<std::string>();
        cond.param2 = stringToOp(node["op"].as<std::string>());
        cond.param3 = node["value"].as<double>();
    } else if (type == "SENSOR_DIFF") {
        cond.type = ConditionType::SENSOR_DIFF;
        cond.param1 = node["sensor_id"].as<std::string>();
        cond.param2 = stringToOp(node["op"].as<std::string>());
        cond.param3 = node["value"].as<double>();
    } else { 
        std::cout << "Error: Unknown condition type: " + type;
        std::cout << " - default to 'IMMEDIATE'" << std::endl;
        cond.type = ConditionType::IMMEDIATE;
    }
    return cond;
}

std::vector<StepAction> Step::parseActions(
        const AutomationContext& context,
        const std::string taskName,
        const YAML::Node& node,
        const std::map<std::string, std::shared_ptr<Action>> actions) {
    std::vector<StepAction> stepActions;
    for (const auto& actionNode : node) {
        StepAction a(taskName, actionNode, context, actions);
        stepActions.push_back(a);
    }
    return stepActions;
}