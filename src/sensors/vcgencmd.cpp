#include "vcgencmd.hpp"

Vcgencmd::Vcgencmd(YAML::Node config)  : Sensor(config) {}

void Vcgencmd::init() {
}

double Vcgencmd::get_value() {
    this->next_read_time += this->read_interval;
#ifndef SIMULATED
    std::string output = exec("vcgencmd get_throttled");
    size_t pos = output.find("0x");
    if (pos != std::string::npos) {
        std::string hex_value_str = output.substr(pos);
        int throttled_value_int = std::stoul(hex_value_str, nullptr, 16);
        return throttled_value_int;
    }
#endif
    return -1;
}

bool Vcgencmd::ready(const TimePoint& now) {
#ifdef SIMULATED
    (void) now;
    return false;
#else
    return due(now);
#endif
}

void Vcgencmd::update(const TimePoint& now) {
    (void) now;
}


std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    auto file_closer = [](FILE* f) {
        if (f) {
            pclose(f);
        }
    };
    
    std::unique_ptr<FILE, decltype(file_closer)> pipe(popen(cmd, "r"), file_closer);

    if (!pipe) {
        return "popen() failed!"; 
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    return result;
}
