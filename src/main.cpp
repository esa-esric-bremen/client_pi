#include <robot_remote_control/ControlledRobot.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <yaml-cpp/yaml.h>

#include <robot_remote_control/Transports/TransportZmq.hpp>
#include <robot_remote_control/Types/RobotRemoteControl.pb.h>

// include hardware driver

#include "actions/action.hpp"
#include "actions/motor_driver.hpp"
#include "actions/gpio_driver.hpp"
#include "automation/automation.hpp"
#include "sensors/weight_sensor.hpp"
#include "sensors/sensor.hpp"
#include "sensors/cpu_temp.hpp"
#include "sensors/power_monitor.hpp"
#include "sensors/vcgencmd.hpp"
#ifdef SIMULATED
#include "driver/dummy.hpp"
#else
#include "driver/gpio.hpp"
#include "driver/hx711.hpp"
#include "driver/ina228.hpp"
#include "driver/l298n.hpp"

#endif

using namespace std::chrono_literals;

using robot_remote_control::TransportSharedPtr;
using robot_remote_control::TransportZmq;


// wait time in microseconds.
// wait 5s for motor controller to get ready
const int MOTOR_INIT_SLEEP = 5000000;
// run a 50ms update loop
const int LOOP_SLEEP =         50000;
// wait 100ms to reconnect
const int CONNECTION_SLEEP =  100000;


void init_drivers(
    YAML::Node config,
    std::map<std::string, std::shared_ptr<Action>>& actions,
    std::map<std::string, std::shared_ptr<Sensor>>& sensors)
{
    std::cout << "initiate drivers:" << std::endl;
#ifndef SIMULATED
    wiringPiSetupGpio();
#endif
    bool any_gpio = false;
    for (const auto& action_conf : config["actions"]) {
        std::string driver = action_conf["driver"].as<std::string>();
        std::string cmd = action_conf["command"].as<std::string>();
        if (driver == "gpio") {
            auto gpio_drv = std::make_shared<GpioDriver>(action_conf);
            gpio_drv->init();
            actions.emplace(cmd, gpio_drv);
            any_gpio = true;
        } else if (driver == "motor") {
            auto motor_driver = std::make_shared<MotorDriver>(action_conf);
            motor_driver->init();
            actions.emplace(cmd, motor_driver);
            any_gpio = true;
        } else {
            std::cout << "Unknown action driver: " << driver << std::endl;
        }
    }
    if (any_gpio) {
        usleep(MOTOR_INIT_SLEEP);
        std::cout << " GPIO ready" << std::endl;
    }
    std::vector<std::thread> init_threads;

    for (const auto& sensor_conf : config["sensors"]) {
        std::string driver = sensor_conf["driver"].as<std::string>();
        std::string sensor_id = sensor_conf["id"].as<std::string>();

        if (driver == "hx711") {
            auto weight = std::make_shared<WeightSensor>(sensor_conf);
            weight->init();
            init_threads.emplace_back(&WeightSensor::threaded_init, weight.get());
            sensors.emplace(sensor_id, weight);
        } else if (driver == "ina228") {
            auto power = std::make_shared<PowerMonitor>(sensor_conf, "ina228");
            power->init();
            sensors.emplace(sensor_id, power);
        } else if (driver == "ads1x15") {
            auto power = std::make_shared<PowerMonitor>(sensor_conf, "ads1x15");
            power->init();
            sensors.emplace(sensor_id, power);
        } else if (driver == "cpu_temp") {
            auto temp = std::make_shared<CpuTemperature>(sensor_conf);
            sensors.emplace(sensor_id, temp);
        } else if (driver == "cpu_throttle" || driver == "vcgencmd") {
            auto vcg = std::make_shared<Vcgencmd>(sensor_conf);
            sensors.emplace(sensor_id, vcg);
        } else {
            std::cout << "Unknown sensor driver: " << driver << std::endl;
        }
    }
    // Wait for all initialization threads to complete
    for (auto& th : init_threads) {
        if (th.joinable()) {
            th.join();
        }
    }
    std::cout << "done." << std::endl;
}

int main(int argc, char** argv)
{
    std::map<std::string, std::shared_ptr<Sensor>> sensors;
    std::map<std::string, std::shared_ptr<Action>> actions;
    std::string filename = "config.yaml";

    if (argc >= 2) {
        filename = argv[1];
    }
    std::cout << "Using " << filename << " config file" << std::endl;

    YAML::Node config = YAML::LoadFile(filename);

    const int cmd_port = config["command_address_port"].as<int>();
    const int tel_port = config["telemetry_address_port"].as<int>();

    std::string cmd = "tcp://*:" + std::to_string(cmd_port);
    std::string tel = "tcp://*:" + std::to_string(tel_port);

    TransportSharedPtr commands = TransportSharedPtr(
        new TransportZmq(cmd, TransportZmq::REP));
    TransportSharedPtr telemetry = TransportSharedPtr(
        new TransportZmq(tel, TransportZmq::PUB));
    robot_remote_control::ControlledRobot robot(commands, telemetry);
#ifdef SIMULATED
    std::cout << "WARNING: Using simulated Dummy driver for Development!" << std::endl;
#endif
    int value = 0;

    // set a system name
    std::string system_name = config["system_name"].as<std::string>();
    robot.initRobotName(system_name); // RPiOnBeneficiation
    robot.setRobotState("INIT");
    
    init_drivers(config, actions, sensors);

    // configure sensors
    robot_remote_control::SimpleSensor sensor;
    sensor.add_value(0); // binary mask which values are updated
    sensor.add_value(0); // current timestamp

    // hold the time when the sensor should be read next
    auto start_time = std::chrono::steady_clock::now();
    for (auto& _sensor : sensors) {
        sensor.add_value(0);
        _sensor.second->init_interval(start_time);
    }
    
    auto automation = std::make_shared<Automation>(config, actions);

    // configure actions
    robot_remote_control::SimpleActions simpleActions;
    robot_remote_control::SimpleAction* action;
    for (auto& _action : actions) {
        action = simpleActions.add_actions();
        action->set_name(_action.second->command);
        action->mutable_type()->set_min_state(_action.second->min);
        action->mutable_type()->set_max_state(_action.second->max);
        action->set_state(_action.second->start);
        _action.second->set_simple_action(action);
        if (_action.second->counter != "") {
            action = simpleActions.add_actions();
            action->set_name(_action.second->counter);
            action->set_state(0);
            _action.second->counterAction = action;
        }
    }

    // we disable automation for now - so we don't accidentally 
    // activate an automation during presentation
    /*
    for (auto& task : automation->tasks) {
        action = simpleActions.add_actions();
        action->set_name("task_" + task.second->name);
        action->set_state(0);
        task.second->set_simple_action(action);
    }
    */

    robot.initSimpleActions(simpleActions);
    robot.startUpdateThread(100);

    std::cout << "Waiting for connection..." << std::endl;
    while (!robot.isConnected()) {
        usleep(CONNECTION_SLEEP);
    }
    std::cout << "Connection established!" << std::endl;

    robot_remote_control::SimpleAction simpleactionscommand;

    while (true) {
        robot.setRobotState("RUNNING");

        while (robot.getSimpleActionCommand(&simpleactionscommand)) {
            value = simpleactionscommand.state();
            std::string cmd_name = simpleactionscommand.name();
            if (cmd_name.rfind("reset_", 0) == 0) {
                std::string cmd_name_ = cmd_name.substr(6);
                if (actions.find(cmd_name_) != actions.end()) {
                    std::shared_ptr<Action>& action_ptr = actions[cmd_name_];
                    std::cout << "Reset " << action_ptr->name << std::endl;
                    action_ptr->reset();
                }
            } else if (cmd_name.rfind("stop_", 0) == 0) {
                std::string cmd_name_ = cmd_name.substr(5);
                if (actions.find(cmd_name_) != actions.end()) {
                    std::shared_ptr<Action>& action_ptr = actions[cmd_name_];
                    std::cout << "Stop " << action_ptr->name << std::endl;
                    action_ptr->stop();
                }
            } else if (cmd_name.rfind("reinit_", 0) == 0) {
                std::string cmd_name_ = cmd_name.substr(7);
                if (actions.find(cmd_name_) != actions.end()) {
                    std::shared_ptr<Action>& action_ptr = actions[cmd_name_];
                    std::cout << "Reinit " << action_ptr->name << std::endl;
                    action_ptr->reinit();
                }
            /*
            } else if (cmd_name.rfind("task_", 0) == 0) {
                std::string task_name = cmd_name.substr(5);
                automation->state(task_name, (int)value);
            */
            } else if (actions.find(cmd_name) != actions.end()) {
                std::shared_ptr<Action>& action_ptr = actions[cmd_name];
                std::cout << "Set " << action_ptr->name <<  " to " << value << std::endl;
                action_ptr->set_value(value);
            } else {
                std::cout << "Ignoring unknown command: " << cmd_name << std::endl;
            }
        }

        robot.initSimpleActions(simpleActions);

        int mask = 0;
        int pos = 0;

        std::map<std::string, double> current_sensor_values;

        // steady clock gets time since program start
        auto now = std::chrono::steady_clock::now();
        // we have to iterate over the sensors in yaml to keep
        // the order, alternative would be to use a vector instead of
        // map.
        for (const auto& sensor_conf : config["sensors"]) {
            std::string sensor_id = sensor_conf["id"].as<std::string>();
            if (sensors.find(sensor_id) == sensors.end()) {
                continue;
            }
            auto& _sensor = sensors[sensor_id];
            if (!_sensor->ready(now)) {
                continue;
            }
            double value = _sensor->get_value();
            current_sensor_values[sensor_id] = value;
            sensor.set_value(pos+2, value);
            mask = mask | _sensor->mask;
            pos++;
        }
        if (mask > 0) {
            auto milliseconds
                = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch()).count();
            float ts = milliseconds/1000.0;
            sensor.set_value(0, mask);
            sensor.set_value(1, ts);
            robot.setSimpleSensor(sensor);
        }

        
        automation->update(current_sensor_values);
        // we need to wait at least 50ms for the hx711 sensor
        // we should wait longer, ~1s is on the safe side
        for (auto& _sensor : sensors) {
            _sensor.second->update(now);
        }
        for (auto& action : actions) {
            action.second->update();
        }
        usleep(LOOP_SLEEP);
    }
    return 0;
}

