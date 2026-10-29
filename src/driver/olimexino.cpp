#include "olimexino.hpp"

#include <cstdlib>
#include <iostream>
#include <sstream>

namespace {
// The firmware's host watchdog (if present) stops all motors after 1 s without a line.
constexpr auto KEEPALIVE_PERIOD = std::chrono::milliseconds(250);
}

Olimexino::~Olimexino() {
    shutdown();
}

void Olimexino::start() {
    running = true;
    start_transport();
    keepalive_thread = std::thread(&Olimexino::keepalive_loop, this);
}

void Olimexino::shutdown() {
    running = false;
    if (keepalive_thread.joinable()) {
        keepalive_thread.join();
    }
}

bool Olimexino::send(const std::string& command) {
    std::lock_guard<std::mutex> lock(send_mutex);
    return write_line(command);
}

std::optional<double> Olimexino::channel(const std::string& name) const {
    std::lock_guard<std::mutex> lock(telemetry_mutex);
    const auto it = telemetry.find(name);
    if (it == telemetry.end() || Clock::now() - it->second.second > TELEMETRY_TIMEOUT) {
        return std::nullopt;
    }
    return it->second.first;
}

bool Olimexino::alive() const {
    std::lock_guard<std::mutex> lock(telemetry_mutex);
    return Clock::now() - last_telemetry <= TELEMETRY_TIMEOUT;
}

// "Name:value<TAB>Name:value..."; malformed fields are skipped, e.g. a partial
// line right after connecting
void Olimexino::on_line(const std::string& line) {
    const auto now = Clock::now();
    std::istringstream fields(line);
    std::string field;
    bool any = false;
    std::lock_guard<std::mutex> lock(telemetry_mutex);
    while (std::getline(fields, field, '\t')) {
        const auto colon = field.find(':');
        if (colon == std::string::npos || colon == 0) {
            continue;
        }
        const std::string value_text = field.substr(colon + 1);
        char* end = nullptr;
        const double value = std::strtod(value_text.c_str(), &end);
        if (end == value_text.c_str()) {
            continue;
        }
        telemetry[field.substr(0, colon)] = {value, now};
        any = true;
    }
    if (any) {
        last_telemetry = now;
    }
}

void Olimexino::keepalive_loop() {
    while (running) {
        send("ping");
        std::this_thread::sleep_for(KEEPALIVE_PERIOD);
    }
}
