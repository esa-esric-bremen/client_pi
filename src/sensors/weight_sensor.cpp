#include "weight_sensor.hpp"

WeightSensor::WeightSensor(YAML::Node config)  : Sensor(config){
    this->gpioPinDT = config["pins"]["dt"].as<int>();
    this->gpioPinSCK = config["pins"]["sck"].as<int>();
    this->zero_value = config["zero_value"].as<int>();
    this->known_weight = config["known_weight"].as<int>();
    this->weight_value = config["weight_value"].as<int>();
    this->runs_start_calibrate = config["start_calibrate_time"].as<int>() * this->frequency;
    this->runs_calibrate = config["calibrate_time"].as<int>() * this->frequency;
}

void WeightSensor::init() {
    hx711_sensor = std::make_unique<HX711>(gpioPinDT, gpioPinSCK);
    hx711_sensor->set_reading_format("MSB", "MSB");
    double reference_unit = -static_cast<double>(weight_value - zero_value) / known_weight;
    std::cout << "set reference unit:" << reference_unit << std::endl;
    hx711_sensor->set_reference_unit(reference_unit);
    is_running_ = true;
    init_done_ = false;
    update_thread_ = std::thread(&WeightSensor::threaded_update, this);
}

void WeightSensor::threaded_init() {
    hx711_sensor->reset();
    hx711_sensor->tare(15);
    std::cout << "HX711 for a sensor is ready" << std::endl;
    std::lock_guard<std::mutex> lock(data_mutex_);
    init_done_ = true;
}

double WeightSensor::get_value() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return weight - this->calibration_weight;
}

bool WeightSensor::ready(const TimePoint& now) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return (this->runs_init >= this->runs_calibrate) && this->due(now);
}

void WeightSensor::threaded_update() {
    while (is_running_) {
        auto now = std::chrono::steady_clock::now();
        if (!due(now) || !init_done_) {
            std::this_thread::sleep_for(
            std::chrono::milliseconds(50ms));
            continue;
        }
        this->next_read_time += this->read_interval;
        if (runs_init < runs_start_calibrate) {
            std::lock_guard<std::mutex> lock(data_mutex_);
            runs_init++;
        } else if (runs_init < runs_calibrate) {
            double _weight = hx711_sensor->get_weight(5);
            std::lock_guard<std::mutex> lock(data_mutex_);
            calibration_weight += _weight / (runs_calibrate - runs_start_calibrate);
            runs_init++;
        } else {
            double _weight = hx711_sensor->get_weight(5);
            hx711_sensor->reset(); // power down and up
            std::lock_guard<std::mutex> lock(data_mutex_);
            weight = _weight;
        }
    }
}

void WeightSensor::update(const TimePoint& now) {
    (void) now;
    return;
}

WeightSensor::~WeightSensor() {
    is_running_ = false;
    if (update_thread_.joinable()) {
        update_thread_.join();
    }
}