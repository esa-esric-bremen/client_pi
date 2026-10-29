// Link to the OLIMEXINO-STM32F3 motor controller
// (CubeMars AK80-6 and SitoDrive TA70-100 on CAN),
// firmware OLIMEXINO_AK80_TA70_*.ino.

#pragma once

#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

#include <yaml-cpp/yaml.h>

class Olimexino {
public:
    using Clock = std::chrono::steady_clock;

    // telemetry older than this counts as missing
    static constexpr std::chrono::milliseconds TELEMETRY_TIMEOUT{500};

    virtual ~Olimexino();

    // Thread-safe. False if the board is not connected.
    bool send(const std::string& command);

    // Latest value of a telemetry channel, if it is current.
    std::optional<double> channel(const std::string& name) const;

    // telemetry is arriving
    bool alive() const;

    // Starts the transport and the keepalive. Call once.
    void start();

protected:
    virtual bool write_line(const std::string& line) = 0;
    virtual void start_transport() = 0;

    // called by the transport for every received line
    void on_line(const std::string& line);

    // Stops the keepalive. Subclasses call this first in their destructor,
    // before their transport goes away.
    void shutdown();

    std::atomic<bool> running{false};

private:
    void keepalive_loop();

    mutable std::mutex telemetry_mutex;
    std::map<std::string, std::pair<double, Clock::time_point>> telemetry;
    Clock::time_point last_telemetry{};

    std::mutex send_mutex;
    std::thread keepalive_thread;
};

// Serial port in real builds, the simulated board in SIMULATED builds.
// config: the `olimexino:` section of the robot config.
std::shared_ptr<Olimexino> make_olimexino(const YAML::Node& config);
