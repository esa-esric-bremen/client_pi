#include "sim_model.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace {

// container and material
constexpr double CONTAINER_CAPACITY_G = 7000.0;  // above max_okay of the scale
constexpr double DIG_RATE_G_S = 120.0;           // at full excavator speed, fully lowered
constexpr double UNLOAD_RATE_G_S = 400.0;        // at full screw speed
constexpr double MIN_DIG_FACTOR = 0.15;          // the bucket scrapes the surface when raised
constexpr double FULL_DEPTH_MS = 8000.0;         // linear actuator run time to full depth

// electrical
constexpr double IDLE_CURRENT_A = 0.35;
constexpr double EXCAVATOR_CURRENT_A = 2.8;
constexpr double SCREW_CURRENT_A = 1.6;
constexpr double LINEAR_CURRENT_A = 0.9;
constexpr double LED_CURRENT_A = 0.004;          // per LED step (0..100)
constexpr double CURRENT_TAU_S = 0.3;
// small on purpose, so the voltage visibly drops within a demo session
constexpr double BATTERY_CAPACITY_AH = 3.0;
constexpr double EMPTY_VOLTAGE_V = 42.0;
constexpr double VOLTAGE_SPAN_V = 8.6;
constexpr double INTERNAL_RESISTANCE_OHM = 0.25;

// thermal
constexpr double CPU_BASE_TEMP_C = 47.0;
constexpr double CPU_TEMP_PER_A = 2.0;
constexpr double CPU_TEMP_TAU_S = 30.0;

// vcgencmd get_throttled bits
constexpr int UNDER_VOLTAGE = 1 << 0;
constexpr int THROTTLED = 1 << 2;
constexpr int SOFT_TEMP_LIMIT = 1 << 3;
constexpr int OCCURRED_SHIFT = 16;
constexpr double UNDER_VOLTAGE_V = 43.0;

double clamp01(double v) {
    return std::clamp(v, 0.0, 1.0);
}

const YAML::Node findAction(const YAML::Node& config, const std::string& command) {
    for (const auto& action : config["actions"]) {
        if (action["command"] && action["command"].as<std::string>() == command) {
            return action;
        }
    }
    return YAML::Node();
}

}  // namespace

SimModel& sim_model() {
    static SimModel model;
    return model;
}

void SimModel::configure(const YAML::Node& config) {
    std::lock_guard<std::mutex> lock(mutex);

    auto configureBldc = [&](const std::string& command, Bldc* bldc) {
        const YAML::Node action = findAction(config, command);
        if (!action) {
            std::cout << "simulation: no action '" << command << "', not simulated" << std::endl;
            return;
        }
        bldc->pin = action["pin"].as<int>();
        bldc->center = action["start"].as<double>();
        for (const auto& zone : action["deadzones"]) {
            for (const auto& edge : zone) {
                bldc->deadband = std::max(bldc->deadband, std::abs(edge.as<double>() - bldc->center));
            }
        }
    };
    configureBldc("excavator_speed", &excavator);
    configureBldc("screw_speed", &screw);

    if (const YAML::Node linear = findAction(config, "linear_motor_1")) {
        linear_en_pin = linear["pins"]["en"].as<int>();
    }
    if (const YAML::Node led = findAction(config, "led_1")) {
        led_pin = led["pin"].as<int>();
    }
    std::cout << "simulation: excavator pin " << excavator.pin << ", screw pin " << screw.pin
              << ", linear actuator pin " << linear_en_pin << ", LED pin " << led_pin << std::endl;
}

// ESC: `center` is neutral, values below it run the motor in working direction
double SimModel::bldcSpeed(const Bldc& bldc, double value) {
    const double offset = bldc.center - value;
    if (std::abs(offset) <= bldc.deadband) {
        return 0.0;
    }
    // full speed 50 steps from neutral (pwm 100/200 for neutral 150)
    return std::clamp(offset / 50.0, -1.0, 1.0);
}

void SimModel::set_pwm(int pin, double value) {
    std::lock_guard<std::mutex> lock(mutex);
    step();
    if (pin == excavator.pin) {
        excavator.speed = bldcSpeed(excavator, value);
    } else if (pin == screw.pin) {
        screw.speed = bldcSpeed(screw, value);
    } else if (pin == led_pin) {
        led_value = value;
    }
}

void SimModel::set_motor(int en_pin, double direction) {
    std::lock_guard<std::mutex> lock(mutex);
    step();
    if (en_pin == linear_en_pin) {
        linear_direction = (direction > 0) - (direction < 0);
    }
}

void SimModel::step() {
    const auto now = std::chrono::steady_clock::now();
    // cap: a stalled caller must not make the model jump
    const double dt = std::min(std::chrono::duration<double>(now - last_step).count(), 1.0);
    last_step = now;

    linear_position_ms += linear_direction * dt * 1000.0;
    const double depth = clamp01(linear_position_ms / FULL_DEPTH_MS);

    double dig = 0.0;
    if (excavator.speed > 0 && weight < CONTAINER_CAPACITY_G) {
        dig = DIG_RATE_G_S * excavator.speed * (MIN_DIG_FACTOR + (1.0 - MIN_DIG_FACTOR) * depth);
    }
    const double unload = (screw.speed > 0 && weight > 0) ? UNLOAD_RATE_G_S * screw.speed : 0.0;
    weight = std::max(0.0, weight + (dig - unload) * dt);

    const double load_current = IDLE_CURRENT_A
        + EXCAVATOR_CURRENT_A * std::abs(excavator.speed) * (0.6 + 0.6 * depth)
        + SCREW_CURRENT_A * std::abs(screw.speed)
        + (linear_direction != 0 ? LINEAR_CURRENT_A : 0.0)
        + LED_CURRENT_A * led_value;
    current += (load_current - current) * (1.0 - std::exp(-dt / CURRENT_TAU_S));

    state_of_charge = std::max(0.0, state_of_charge - current * dt / 3600.0 / BATTERY_CAPACITY_AH);

    const double target_temp = CPU_BASE_TEMP_C + CPU_TEMP_PER_A * current;
    cpu_temp += (target_temp - cpu_temp) * (1.0 - std::exp(-dt / CPU_TEMP_TAU_S));
}

double SimModel::noise(double sigma) {
    return std::normal_distribution<double>(0.0, sigma)(rng);
}

double SimModel::weight_g() {
    std::lock_guard<std::mutex> lock(mutex);
    step();
    // load cells pick up the vibration of the running excavator
    return weight + noise(3.0 + 25.0 * std::abs(excavator.speed));
}

double SimModel::current_a() {
    std::lock_guard<std::mutex> lock(mutex);
    step();
    return std::max(0.0, current + noise(0.03));
}

double SimModel::voltage_v() {
    std::lock_guard<std::mutex> lock(mutex);
    step();
    return EMPTY_VOLTAGE_V + VOLTAGE_SPAN_V * state_of_charge - INTERNAL_RESISTANCE_OHM * current + noise(0.03);
}

double SimModel::cpu_temp_c() {
    std::lock_guard<std::mutex> lock(mutex);
    step();
    return cpu_temp + noise(0.2);
}

int SimModel::throttled() {
    std::lock_guard<std::mutex> lock(mutex);
    step();
    int now = 0;
    if (EMPTY_VOLTAGE_V + VOLTAGE_SPAN_V * state_of_charge - INTERNAL_RESISTANCE_OHM * current < UNDER_VOLTAGE_V) {
        now |= UNDER_VOLTAGE;
    }
    if (cpu_temp > 60.0) {
        now |= SOFT_TEMP_LIMIT;
    }
    if (cpu_temp > 80.0) {
        now |= THROTTLED;
    }
    throttle_sticky |= now << OCCURRED_SHIFT;
    return now | throttle_sticky;
}
