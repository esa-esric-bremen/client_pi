#include "olimexino_actions.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <stdexcept>

namespace {

// firmware limits (OLIMEXINO_AK80_TA70_*.ino)
constexpr double AK_MAX_ERPM = 100000;
constexpr double TA_MIN_DEG = 0.0;
constexpr double TA_MAX_DEG = 45.0;
constexpr int TA_MIN_RPM = 1, TA_MAX_RPM = 4000;
constexpr int TA_MIN_ACCEL = 50, TA_MAX_ACCEL = 10000;
constexpr int TA_MIN_CURRENT = 500, TA_MAX_CURRENT = 5000;
// TA_TargetDeg has 3 decimals, the drive resolution is 360/65536 deg
constexpr double TA_CONFIRM_TOLERANCE_DEG = 0.01;
// interval to resend rejected targets
constexpr auto TA_RESEND = std::chrono::milliseconds(300);
constexpr int TA_PROFILE_POSITION_MODE = 7;

void read_common(Action& action, const YAML::Node& config) {
    action.name = config["name"].as<std::string>();
    action.command = config["command"].as<std::string>();
    action.driver = config["driver"].as<std::string>();
    const auto range = config["range"].as<std::vector<double>>();
    if (range.size() != 2 || range[0] >= range[1]) {
        throw std::runtime_error(
            action.command + ": range must be [min, max]");
    }
    action.min = range[0];
    action.max = range[1];
    action.start = config["start"] ? config["start"].as<double>() : 0.0;
    if (action.start < action.min || action.start > action.max) {
        throw std::runtime_error(
            action.command + ": start outside of range");
    }
    action.value = action.start;
}

int read_int(
        const YAML::Node& config,
        const std::string& key,
        int fallback,
        int low,
        int high,
        const std::string& command) {
    const int value = config[key] ? config[key].as<int>() : fallback;
    if (value < low || value > high) {
        throw std::runtime_error(
            command + ": " + key + " must be " + std::to_string(low) + 
            ".." + std::to_string(high));
    }
    return value;
}

std::string format_deg(double deg) {
    char text[32];
    std::snprintf(text, sizeof(text), "%.3f", deg);
    return text;
}

}  // namespace

// AK80-6

AkSpeedAction::AkSpeedAction(
        const YAML::Node& config, std::shared_ptr<Olimexino> board)
    : board(std::move(board)) {
    read_common(*this, config);
    if (min < -AK_MAX_ERPM || max > AK_MAX_ERPM) {
        throw std::runtime_error(
            command + ": range must be within +/-100000 ERPM (firmware limit)");
    }
}

void AkSpeedAction::init() {
    std::cout << "  " << command << ": AK80-6 via olimexino" << std::endl;
}

void AkSpeedAction::set_value(double requested) {
    value = std::clamp(requested, min, max);
    board->send("ak " + std::to_string(std::lround(value)));
}

void AkSpeedAction::update() {
    if (const auto target = board->channel("AK_TargetERPM")) {
        simpleAction->set_state(*target);
    }
}

void AkSpeedAction::stop() {
    value = 0;
    board->send("ak stop");
}

void AkSpeedAction::reset() {
    stop();
}

void AkSpeedAction::reinit() {
    value = 0;
    board->send("ak idle");
}

// TA70-100

TaPositionAction::TaPositionAction(
        const YAML::Node& config,
        std::shared_ptr<Olimexino> board)
    : board(std::move(board)) {
    read_common(*this, config);
    if (min < TA_MIN_DEG || max > TA_MAX_DEG) {
        throw std::runtime_error(
            command + ": range must be within 0..45 deg (firmware limit)");
    }
    speed_rpm = read_int(
        config, 
        "speed_rpm", 
        300,
         TA_MIN_RPM, 
         TA_MAX_RPM,
         command);
    accel_rpm_s = read_int(
        config,
        "accel_rpm_s",
        500,
        TA_MIN_ACCEL,
        TA_MAX_ACCEL,
        command);
    current_ma = read_int(
        config,
        "current_ma",
        3000,
        TA_MIN_CURRENT,
        TA_MAX_CURRENT,
        command);
}

void TaPositionAction::init() {
    // entering position mode can release the brake and move the actuator,
    // so it is never done automatically: the operator uses "init" (reinit)
    std::cout << "  " << command << ": TA70-100 via olimexino, needs init before moving" << std::endl;
}

bool TaPositionAction::ready() {
    return !pending.has_value();
}

void TaPositionAction::set_value(double requested) {
    value = std::clamp(requested, min, max);
    pending = value;
    warned_not_in_mode = false;
    last_sent = {};  // send right away
}

void TaPositionAction::update() {
    if (const auto actual = board->channel("TA_ActualDeg")) {
        simpleAction->set_state(*actual);
    }
    if (!pending) {
        return;
    }
    const auto confirmed = board->channel("TA_TargetDeg");
    if (confirmed && std::abs(*confirmed - *pending) <= TA_CONFIRM_TOLERANCE_DEG) {
        pending.reset();
        return;
    }
    if (Clock::now() - last_sent < TA_RESEND) {
        return;
    }

    const auto mode = board->channel("TA_Mode");
    if (mode && static_cast<int>(*mode) != TA_PROFILE_POSITION_MODE) {
        if (!warned_not_in_mode) {
            std::cout << command << ": TA70 is not in position mode, target "
                      << *pending << " deg waits for init" << std::endl;
            warned_not_in_mode = true;
        }
        return;
    }
    const auto moving = board->channel("TA_MoveActive");
    if (moving && *moving != 0) {
        return;  // the previous move has to settle first
    }
    board->send("ta pos " + format_deg(*pending) + " " + std::to_string(speed_rpm));
    last_sent = Clock::now();
}

void TaPositionAction::stop() {
    pending.reset();
    board->send("ta stop");
}

void TaPositionAction::reset() {
    pending.reset();
}

void TaPositionAction::reinit() {
    pending.reset();
    board->send("ta current " + std::to_string(current_ma));
    board->send("ta accel " + std::to_string(accel_rpm_s));
    board->send("ta mode pos");
}
