#include "motor_driver.hpp"

MotorDriver::MotorDriver(YAML::Node config) {
    this->positionTime = 0;
    this->desiredTime = 0;
    this->direction = 0;

    this->name = config["name"].as<std::string>();
    this->command = config["command"].as<std::string>();
    this->driver = config["driver"].as<std::string>();
    this->control = config["control"].as<std::string>();

    this->en = config["pins"]["en"].as<int>();
    this->in1 = config["pins"]["in1"].as<int>();
    this->in2 = config["pins"]["in2"].as<int>();
    this->mode = config["mode"].as<std::string>();
    this->pwm = this->mode == "pwm";

    this->start = config["start"].as<int>();
    this->min = config["range"].as<std::vector<int>>()[0];
    this->max = config["range"].as<std::vector<int>>()[1];
    if (config["counter"]) {
        this->counter = config["counter"].as<std::string>();
    }
    auto now = std::chrono::steady_clock::now();
    this->lastTime = now;
}

void MotorDriver::init() {
    std::cout << "  using pins ";
    std::cout << this->en << " " << this->in1 << " " << this->in2 << " " << this->pwm;
    std::cout << " for time motor" << std::endl;
#ifndef SIMULATED
    int range = std::max(this->max, std::abs(this->min));
    init_l298n(this->en, this->in1, this->in2, this->pwm, range);
#endif
}

void MotorDriver::reinit() {
    init();
    reset();
}

bool MotorDriver::ready() {
    return true;
}

void MotorDriver::set_value(double val) {
    if (this->control == "position") {
        simpleAction->set_state(val);
        set_l298n(en, in1, in2, val, pwm);
        return;
    }
    // else: control == duration
    this->desiredTime = val;
    if (desiredTime == positionTime) {
        direction = 0;
        set_l298n(en, in1, in2, 0, pwm);
        return;
    }
    direction = desiredTime > positionTime ? 1 : -1;
}

void MotorDriver::stop() {
    if (direction == 0 || this->control == "position") {
        direction = 0;
        set_l298n(en, in1, in2, 0, pwm);
        return;
    }
    // calculate for how long the motor has run
    auto now = std::chrono::steady_clock::now();
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    auto lastTime_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        lastTime.time_since_epoch()).count();
    auto elapsedTime = now_ms - lastTime_ms;
    positionTime += elapsedTime * direction;

    // and now the actual stop
    direction = 0;
    this->desiredTime = positionTime;
    simpleAction->set_state(positionTime);
    set_l298n(en, in1, in2, 0, pwm);
    lastTime = now;
}

void MotorDriver::update() {
    if (this->control == "position") {
        return;
    }
    auto now = std::chrono::steady_clock::now();
    if (direction == 0) {
        lastTime = now;
        return;
    }

    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    // calculate duration we've run the motor
    auto lastTime_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        lastTime.time_since_epoch()).count();
    auto elapsedTime = now_ms - lastTime_ms;
    lastTime = now;

    // calculate for how long the motor has run
    positionTime += elapsedTime * direction;

    bool reachedTarget = false;
    if (direction == 1 && positionTime >= desiredTime) {
        positionTime = desiredTime;
        reachedTarget = true;
    } else if (direction == -1 && positionTime <= desiredTime) {
        positionTime = desiredTime;
        reachedTarget = true;
    }

    if (reachedTarget) {
        // We arrived! Stop the motor!
        direction = 0;
    }
    simpleAction->set_state(positionTime);
    set_l298n(en, in1, in2, direction, pwm);
}

void MotorDriver::reset() {
    lastTime = std::chrono::steady_clock::now();
    direction = 0;
    desiredTime = positionTime;
}