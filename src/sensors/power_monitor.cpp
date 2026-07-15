#include "power_monitor.hpp"
#include <iostream>
#include <string>

std::unique_ptr<INA228> ina228_sensor;
std::unique_ptr<ADS1X15> ads1x15_sensor;

PowerMonitor::PowerMonitor(YAML::Node config, std::string driver) : Sensor(config) {
    this->driver = driver;
    if (!config["address"]) {
        std::cerr << "Address not set for " << this->name << std::endl;
    }
    this->addr = config["address"].as<int>();
    if (!config["reading"]) {
        std::cerr << "Reading not set for " << this->name << std::endl;
    }
    this->reading = config["reading"].as<std::string>();
    // TODO: channel parsing for ads1x15
    // this->max_current = config["max_current"].as<double>();
    // this->r_shunt = config["r_shunt"].as<double>();
    // TODO: gain
    // TODO: bits

    if (config["variant"]) {
        this->variant = config["variant"].as<std::string>();
    }
}

void PowerMonitor::init() {
    if (this->driver == "ina228" && !ina228_sensor) {
        ina228_sensor = std::make_unique<INA228>(this->addr);
        if (ina228_sensor->begin()) {
            // default is INA228_MODE_CONT_TEMP_BUS_SHUNT
            ina228_sensor->setMode(INA228_MODE_CONT_BUS_SHUNT);
            // default is INA228_1_SAMPLE
            // ina228_sensor->setAverage(INA228_128_SAMPLES);
            ina228_sensor->setMaxCurrentShunt(this->max_current, this->r_shunt);
            std::cout << " INA228 ready" << std::endl;
            
        } else {
            std::cout << " ERROR: INA228 not ready (" << this->addr << ")" << std::endl;
        }
    } else if (this->driver == "ads1x15" && !ads1x15_sensor) {
        ads1x15_sensor = std::make_unique<ADS1X15>(
            this->addr, this->variant, 4.096);
    }
}

double PowerMonitor::get_value() {
    this->next_read_time += this->read_interval;
    if (this->driver == "ina228") {
        float bus_voltage = ina228_sensor->getBusVoltage();
        float shunt_voltage = ina228_sensor->getShuntVoltage() * 1000; // in mV
        float current = ina228_sensor->getCurrent() * 1000; // in mA
        float power = ina228_sensor->getPower(); // in W
        std::cout << "Bus Voltage:   " << std::setw(8) << bus_voltage   << " V\t"
                    << "Shunt Voltage: " << std::setw(8) << shunt_voltage << " mV\t"
                    << "Current:       " << std::setw(8) << current       << " mA\t"
                    << "Power:         " << std::setw(8) << power         << " W"
                    << std::endl;
        return bus_voltage;
    }
    else if (this->driver == "ads1x15") {
        double voltage = ads1x15_sensor->getVoltage();
        double current = ads1x15_sensor->getCurrent();
        // std::cout << "voltage: " << voltage << " amps: " << current << std::endl;
        if (this->reading == "voltage") {
            return voltage;
        } else if (this->reading == "current") {
            return current;
        }

    }
    return -1;
}

bool PowerMonitor::ready(const TimePoint& now) {
    return due(now);
}

void PowerMonitor::update(const TimePoint& now) {
    (void) now;
}