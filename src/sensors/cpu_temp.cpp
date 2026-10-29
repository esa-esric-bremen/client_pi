#include "cpu_temp.hpp"
#ifdef SIMULATED
#include "../driver/dummy.hpp"
#endif

CpuTemperature::CpuTemperature(YAML::Node config) : Sensor(config) {
    this->filename = config["filename"].as<std::string>();
    this->scale = config["scale"].as<double>();
}

void CpuTemperature::init() {
}

double CpuTemperature::get_value() {
#ifdef SIMULATED
    this->next_read_time += this->read_interval;
    return read_cpu_temperature();
#else
    std::ifstream tempFile(this->filename);
    if (!tempFile.is_open()) {
        std::cerr << "Failed to open temperature file." << std::endl;
        return -1;
    }
    double temp;
    tempFile >> temp;
    tempFile.close();

    this->next_read_time += this->read_interval;

    return temp * this->scale;
#endif
}

bool CpuTemperature::ready(const TimePoint& now) {
    return due(now);
}

void CpuTemperature::update(const TimePoint& now) {
    (void) now;
}
