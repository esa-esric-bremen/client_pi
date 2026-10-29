// Simulated Coyote 3 robot for offline development.

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <sstream>
#include <thread>

#include <yaml-cpp/yaml.h>

#include <robot_remote_control/ControlledRobot.hpp>
#include <robot_remote_control/Transports/TransportZmq.hpp>
#include <robot_remote_control/Types/RobotRemoteControl.pb.h>

using namespace robot_remote_control;

namespace {

constexpr auto LOOP_PERIOD = std::chrono::milliseconds(50);
constexpr int TELEMETRY_EVERY = 2;          // loops, 10 Hz
constexpr double COMMAND_TIMEOUT_S = 0.5;
constexpr double MAX_LINEAR = 0.8;          // m/s
constexpr double MAX_ANGULAR = 1.2;         // rad/s
constexpr double LINEAR_ACCEL = 0.5;        // m/s^2
constexpr double ANGULAR_ACCEL = 1.5;       // rad/s^2
constexpr double STANDING_EPS = 0.01;

double approach(double value, double target, double max_step) {
    return value + std::clamp(target - value, -max_step, max_step);
}

SimpleActions simpleActionsFromConfig(const YAML::Node& config) {
    SimpleActions actions;
    for (const auto& conf : config["actions"]) {
        if (conf["driver"].as<std::string>() != "rrc") {
            continue;
        }
        auto* action = actions.add_actions();
        action->set_name(conf["command"].as<std::string>());
        action->mutable_type()->set_min_state(conf["range"][0].as<double>());
        action->mutable_type()->set_max_state(conf["range"][1].as<double>());
        action->set_state(conf["start"].as<double>());
    }
    return actions;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " <config.yaml>" << std::endl;
        return 1;
    }
    const YAML::Node config = YAML::LoadFile(argv[1]);
    const std::string cmd = "tcp://*:" + std::to_string(config["command_address_port"].as<int>());
    const std::string tel = "tcp://*:" + std::to_string(config["telemetry_address_port"].as<int>());

    ControlledRobot robot(
        std::make_shared<TransportZmq>(cmd, TransportZmq::REP),
        std::make_shared<TransportZmq>(tel, TransportZmq::PUB));
    robot.initRobotName(config["system_name"].as<std::string>());

    SimpleActions actions = simpleActionsFromConfig(config);
    robot.initSimpleActions(actions);
    robot.startUpdateThread(10);
    std::cout << "simulated Coyote 3 on " << cmd << ", " << tel << std::endl;

    Twist target;
    Twist velocity;
    double x = 0, y = 0, yaw = 0;
    bool standing = true;
    auto last_command = std::chrono::steady_clock::now() - std::chrono::seconds(10);
    const double dt = std::chrono::duration<double>(LOOP_PERIOD).count();

    for (int loop = 0;; ++loop) {
        const auto now = std::chrono::steady_clock::now();

        Twist command;
        if (robot.getTwistCommand(&command)) {
            target.mutable_linear()->set_x(std::clamp<double>(command.linear().x(), -MAX_LINEAR, MAX_LINEAR));
            target.mutable_angular()->set_z(std::clamp<double>(command.angular().z(), -MAX_ANGULAR, MAX_ANGULAR));
            last_command = now;
        }
        const bool timed_out = std::chrono::duration<double>(now - last_command).count() > COMMAND_TIMEOUT_S;
        if (timed_out && (target.linear().x() != 0 || target.angular().z() != 0)) {
            target.Clear();
            robot.setLogMessage(WARN, "no drive command for 0.5 s, stopping");
        }

        SimpleAction action;
        while (robot.getSimpleActionCommand(&action)) {
            for (auto& known : *actions.mutable_actions()) {
                if (known.name() == action.name()) {
                    known.set_state(std::clamp<double>(action.state(), known.type().min_state(), known.type().max_state()));
                    std::cout << known.name() << " = " << known.state() << std::endl;
                }
            }
            robot.initSimpleActions(actions);
        }

        velocity.mutable_linear()->set_x(approach(velocity.linear().x(), target.linear().x(), LINEAR_ACCEL * dt));
        velocity.mutable_angular()->set_z(approach(velocity.angular().z(), target.angular().z(), ANGULAR_ACCEL * dt));
        yaw += velocity.angular().z() * dt;
        x += velocity.linear().x() * std::cos(yaw) * dt;
        y += velocity.linear().x() * std::sin(yaw) * dt;

        const bool now_standing = std::abs(velocity.linear().x()) < STANDING_EPS
            && std::abs(velocity.angular().z()) < STANDING_EPS;
        if (now_standing != standing) {
            standing = now_standing;
            std::ostringstream message;
            message.precision(2);
            message << std::fixed;
            if (standing) {
                message << "stopped at x=" << x << " m, y=" << y << " m, heading=" << yaw * 180.0 / M_PI << " deg";
            } else {
                message << "driving";
            }
            robot.setLogMessage(INFO, message.str());
            robot.setRobotState(standing ? "STANDING" : "DRIVING");
        }

        if (loop % TELEMETRY_EVERY == 0) {
            Pose pose;
            pose.mutable_position()->set_x(x);
            pose.mutable_position()->set_y(y);
            pose.mutable_orientation()->set_z(std::sin(yaw / 2));
            pose.mutable_orientation()->set_w(std::cos(yaw / 2));
            robot.setCurrentPose(pose);
            robot.setCurrentTwist(velocity);
        }
        std::this_thread::sleep_until(now + LOOP_PERIOD);
    }
}
