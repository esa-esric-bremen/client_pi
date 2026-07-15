#pragma once
#include <string>
#include <yaml-cpp/yaml.h>
#include <robot_remote_control/Types/RobotRemoteControl.pb.h>

class Action {
public:
    std::string name;
    std::string command;
    std::string driver;
    double min = -1000.0;
    double max = 1000.0;
    double start = 0.0;
    double value = 0.0;
    std::string counter = "";

    virtual ~Action() {}
    virtual void init() = 0;
    virtual void set_value(double value) = 0;
    virtual void update() = 0;
    virtual bool ready() = 0;
    virtual void stop() = 0;
    virtual void reset() = 0;
    virtual void reinit() = 0;
    void set_simple_action(robot_remote_control::SimpleAction* simpleAction) {
        this->simpleAction = simpleAction;
    };
    robot_remote_control::SimpleAction* counterAction;

protected:
    robot_remote_control::SimpleAction* simpleAction;

};