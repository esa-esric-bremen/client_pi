// Simulated OLIMEXINO
#include "olimexino.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <sstream>

namespace {

constexpr auto STEP = std::chrono::milliseconds(50);
constexpr double TA_MIN_DEG = 0.0;
constexpr double TA_MAX_DEG = 45.0;
constexpr double TA_REDUCER_RATIO = 100.0; // TA70-100
constexpr double TA_SETTLE_DEG = 0.5;
constexpr auto TA_SETTLE_DWELL = std::chrono::milliseconds(300);
constexpr auto TA_MODE_SWITCH = std::chrono::milliseconds(900);  // reset wait + confirmation
constexpr double AK_MAX_ERPM = 100000;
constexpr double AK_ACCEL_ERPM_S = 30000;
constexpr auto HOST_TIMEOUT = std::chrono::milliseconds(1000);

class SimulatedOlimexino : public Olimexino {
public:
    ~SimulatedOlimexino() override {
        shutdown();
        if (model.joinable()) {
            model.join();
        }
    }

protected:
    void start_transport() override {
        model = std::thread(&SimulatedOlimexino::run, this);
        std::cout << "olimexino: SIMULATED board" << std::endl;
    }

    bool write_line(const std::string& line) override {
        std::lock_guard<std::mutex> lock(state_mutex);
        last_host_line = Clock::now();
        host_timed_out = false;
        handle(line);
        return true;
    }

private:
    // commands, same parsing rules as handleCommand() in the firmware
    void handle(const std::string& cmd) {
        std::istringstream in(cmd);
        std::string a, b;
        in >> a >> b;
        if (cmd == "ping" || cmd == "status" || cmd == "help") {
        } else if (cmd == "ak stop") {
            ak_target = 0;
            ak_active = true;
        } else if (cmd == "ak idle") {
            ak_target = 0;
            ak_active = false;
        } else if (a == "ak") {
            const double erpm = std::strtod(b.c_str(), nullptr);
            if (std::abs(erpm) <= AK_MAX_ERPM) {
                ak_target = erpm;
                ak_active = true;
            }
        } else if (cmd == "ta mode pos") {
            if (ak_active && ak_target != 0) {
                return;  // firmware: stop AK first
            }
            ta_mode = 0;
            ta_brake = 1;
            ta_move_active = false;
            ta_switch_until = Clock::now() + TA_MODE_SWITCH;
        } else if (a == "ta" && (b == "pos" || b == "rel")) {
            double deg = 0;
            long rpm = 0;
            in >> deg >> rpm;
            if (in.fail() || ta_mode != 7 || ta_move_active || rpm < 1 || rpm > 4000) {
                return;  // firmware ignores the target
            }
            if (b == "rel") {
                deg += ta_actual;
            }
            ta_target = std::clamp(deg, TA_MIN_DEG, TA_MAX_DEG);
            ta_speed_deg_s = rpm / TA_REDUCER_RATIO * 360.0 / 60.0;
            ta_move_active = true;
            ta_settled_since.reset();
        } else if (cmd == "ta stop") {
            ta_abort();
        } else if (cmd == "ta brake lock" || cmd == "ta brake release") {
            if (ta_mode == 7 && !ta_move_active) {
                ta_brake = cmd == "ta brake lock" ? 1 : 0;
            }
        } else if (a == "ta" && (b == "accel" || b == "current" || b == "autozero")) {
            // RAM settings, no visible effect here
        } else if (cmd == "stop all" || cmd == "all stop" || cmd == "estop") {
            stop_all();
        }
    }

    void ta_abort() {
        ta_mode = 0;
        ta_brake = 1;
        ta_move_active = false;
        ta_switch_until.reset();
    }

    void stop_all() {
        ta_abort();
        ak_target = 0;
        ak_active = false;
    }

    // --- physics and telemetry ---
    void run() {
        auto next = Clock::now();
        while (running) {
            next += STEP;
            std::string line;
            {
                std::lock_guard<std::mutex> lock(state_mutex);
                step(std::chrono::duration<double>(STEP).count());
                line = telemetry_line();
            }
            on_line(line);
            std::this_thread::sleep_until(next);
        }
    }

    void step(double dt) {
        const auto now = Clock::now();
        if (last_host_line && !host_timed_out && now - *last_host_line > HOST_TIMEOUT) {
            host_timed_out = true;
            stop_all();
        }

        if (ta_switch_until && now >= *ta_switch_until) {
            ta_switch_until.reset();
            ta_mode = 7;
            ta_brake = 0;  // mode entry releases the brake
            ta_target = ta_actual;
        }
        if (ta_mode == 7 && ta_move_active) {
            const double error = ta_target - ta_actual;
            ta_actual += std::clamp(error, -ta_speed_deg_s * dt, ta_speed_deg_s * dt);
            if (std::abs(ta_target - ta_actual) <= TA_SETTLE_DEG) {
                if (!ta_settled_since) {
                    ta_settled_since = now;
                }
                if (now - *ta_settled_since >= TA_SETTLE_DWELL) {
                    ta_actual = ta_target;
                    ta_move_active = false;
                }
            }
        }

        const double target = ak_active ? ak_target : 0.0;
        ak_actual += std::clamp(target - ak_actual, -AK_ACCEL_ERPM_S * dt, AK_ACCEL_ERPM_S * dt);
        const double load = std::abs(ak_actual) / 20000.0;
        ak_current = ak_active || std::abs(ak_actual) > 1 ? 0.3 + 4.0 * load : 0.0;
        // slow heating towards 30 °C + 6 °C per ampere
        ak_temperature += (30.0 + 6.0 * ak_current - ak_temperature) * dt / 120.0;
    }

    std::string telemetry_line() const {
        char line[400];
        std::snprintf(line, sizeof(line),
            "TA_TargetDeg:%.3f\tTA_ActualDeg:%.3f\tAK_TargetERPM:%d\tAK_ActualERPM:%d\tAK_CurrentA:%.3f\t"
            "AK_TemperatureC:%d\tTA_BrakeLocked:%d\tTA_Mode:%d\tTA_MoveActive:%d\tTA_FeedbackAlive:1\t"
            "TA_Error:0\tAK_FeedbackAlive:1\tAK_Error:0\tHostAlive:%d",
            ta_target, ta_actual, static_cast<int>(ak_target), static_cast<int>(std::lround(ak_actual)),
            ak_current, static_cast<int>(ak_temperature), ta_brake, ta_mode, ta_move_active ? 1 : 0,
            host_timed_out || !last_host_line ? 0 : 1);
        return line;
    }

    std::mutex state_mutex;
    std::thread model;

    std::optional<Clock::time_point> last_host_line;
    bool host_timed_out = false;

    double ak_target = 0, ak_actual = 0, ak_current = 0, ak_temperature = 30;
    bool ak_active = false;

    int ta_mode = 0;
    int ta_brake = -1;  // unknown after boot, like the firmware
    double ta_target = 0, ta_actual = 0, ta_speed_deg_s = 1;
    bool ta_move_active = false;
    std::optional<Clock::time_point> ta_switch_until;
    std::optional<Clock::time_point> ta_settled_since;
};

}  // namespace

std::shared_ptr<Olimexino> make_olimexino(const YAML::Node& /*config*/) {
    auto board = std::make_shared<SimulatedOlimexino>();
    board->start();
    return board;
}
