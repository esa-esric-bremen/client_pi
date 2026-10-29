// Actions for the motors behind the OLIMEXINO (see driver/olimexino.hpp).
#pragma once

#include <chrono>
#include <memory>
#include <optional>

#include "action.hpp"
#include "../driver/olimexino.hpp"

// CubeMars AK80-6, speed in ERPM (driver: olimexino_ak80).
class AkSpeedAction : public Action {
public:
    AkSpeedAction(const YAML::Node& config, std::shared_ptr<Olimexino> board);
    void init() override;
    void set_value(double value) override;
    void update() override;
    bool ready() override { return true; }
    void stop() override;
    void reset() override;
    // set current to 0: no torque at all
    void reinit() override;

private:
    std::shared_ptr<Olimexino> board;
};

// SitoDrive TA70-100, angle in degrees (driver: olimexino_ta70).
class TaPositionAction : public Action {
public:
    TaPositionAction(const YAML::Node& config, std::shared_ptr<Olimexino> board);
    void init() override;
    void set_value(double value) override;
    void update() override;
    bool ready() override;
    // abort the move (drive reset, brake locks); 
    // needs init (reinit) afterwards!
    void stop() override;
    void reset() override;
    void reinit() override;

private:
    using Clock = std::chrono::steady_clock;

    std::shared_ptr<Olimexino> board;
    int speed_rpm;
    int accel_rpm_s;
    int current_ma;

    std::optional<double> pending;
    Clock::time_point last_sent{};
    bool warned_not_in_mode = false;
};
