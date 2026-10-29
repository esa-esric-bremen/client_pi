// Physical model of the excavation system for SIMULATED builds.
//
// The dummy driver forwards actuator commands here and reads sensor values from
// here, so sensors react to what the operator does:
//   - the bucket excavator fills the container, faster when it is lowered
//   - the screw empties the container
//   - motor load shows in the current, current drains the battery voltage
//   - CPU temperature follows the load, throttle flags follow temperature/voltage
//
// Actuators are identified by the pins of their actions in the config
// (excavator_speed, screw_speed, linear_motor_1, led_1); missing actions are
// simply not simulated.
#pragma once

#include <chrono>
#include <mutex>
#include <random>

#include <yaml-cpp/yaml.h>

class SimModel {
public:
    void configure(const YAML::Node& config);

    // actuators
    void set_pwm(int pin, double value);
    void set_motor(int en_pin, double direction);

    // sensors
    double weight_g();
    double current_a();
    double voltage_v();
    double cpu_temp_c();
    int throttled();

private:
    struct Bldc {
        int pin = -1;
        double center = 150;    // ESC neutral
        double deadband = 0;    // half width around center without motion
        double speed = 0;       // -1..1, positive = working direction
    };

    void step();  // advance the model to now; caller holds mutex
    static double bldcSpeed(const Bldc& bldc, double value);
    double noise(double sigma);

    std::mutex mutex;
    std::chrono::steady_clock::time_point last_step = std::chrono::steady_clock::now();
    std::mt19937 rng{42};

    Bldc excavator;
    Bldc screw;
    int linear_en_pin = -1;
    int led_pin = -1;

    double linear_direction = 0;   // -1, 0, 1 (1 = lowering)
    double linear_position_ms = 0;
    double led_value = 0;

    double weight = 0;             // g in the container
    double current = 0.35;         // A, follows the load with a lag
    double state_of_charge = 0.9;
    double cpu_temp = 46.0;
    int throttle_sticky = 0;
};

SimModel& sim_model();
