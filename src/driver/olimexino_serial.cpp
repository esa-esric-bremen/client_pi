// USB serial driver for the OLIMEXINO.
#include "olimexino.hpp"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <stdexcept>

#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>

namespace {

constexpr auto REOPEN_PERIOD = std::chrono::seconds(1);
constexpr int POLL_TIMEOUT_MS = 200;
// 1024 should be enough, the firmware sends ~150 characters
constexpr size_t MAX_LINE = 1024;

class SerialOlimexino : public Olimexino {
public:
    explicit SerialOlimexino(std::string device) : device(std::move(device)) {}

    ~SerialOlimexino() override {
        shutdown();
        if (reader.joinable()) {
            reader.join();
        }
        close_port();
    }

protected:
    void start_transport() override {
        reader = std::thread(&SerialOlimexino::read_loop, this);
    }

    bool write_line(const std::string& line) override {
        std::lock_guard<std::mutex> lock(fd_mutex);
        if (fd < 0) {
            return false;
        }
        const std::string data = line + "\n";
        if (::write(fd, data.data(), data.size()) != static_cast<ssize_t>(data.size())) {
            return false;
        }
        return true;
    }

private:
    bool open_port() {
        const int new_fd = ::open(device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (new_fd < 0) {
            return false;
        }
        termios tty{};
        if (tcgetattr(new_fd, &tty) != 0) {
            ::close(new_fd);
            return false;
        }
        cfmakeraw(&tty);
        cfsetispeed(&tty, B115200);
        cfsetospeed(&tty, B115200);
        tty.c_cflag |= CLOCAL | CREAD;
        tty.c_cflag &= ~CRTSCTS;
        if (tcsetattr(new_fd, TCSANOW, &tty) != 0) {
            ::close(new_fd);
            return false;
        }
        tcflush(new_fd, TCIOFLUSH);
        std::lock_guard<std::mutex> lock(fd_mutex);
        fd = new_fd;
        return true;
    }

    void close_port() {
        std::lock_guard<std::mutex> lock(fd_mutex);
        if (fd >= 0) {
            ::close(fd);
            fd = -1;
        }
    }

    void lost() {
        std::cerr << "olimexino: connection to " << device << " lost" << std::endl;
        close_port();
    }

    void read_loop() {
        bool reported_missing = false;
        std::string buffer;
        while (running) {
            int current;
            {
                std::lock_guard<std::mutex> lock(fd_mutex);
                current = fd;
            }
            if (current < 0) {
                if (!open_port()) {
                    if (!reported_missing) {
                        std::cerr << "olimexino: cannot open " << device << 
                            " (" << std::strerror(errno)
                            << "), retrying" << std::endl;
                        reported_missing = true;
                    }
                    std::this_thread::sleep_for(REOPEN_PERIOD);
                    continue;
                }
                std::cout << "olimexino: connected on " << device << std::endl;
                reported_missing = false;
                buffer.clear();
                continue;
            }

            pollfd pfd{current, POLLIN, 0};
            const int ready = ::poll(
                &pfd, 1, POLL_TIMEOUT_MS);
            if (ready == 0) {
                continue;
            }
            if (ready < 0) {
                if (errno != EINTR) {
                    lost();
                }
                continue;
            }
            if (!(pfd.revents & POLLIN)) {
                // POLLHUP/POLLERR without data: unplugged or reset
                lost();
                continue;
            }
            char chunk[256];
            const ssize_t n = ::read(
                current, chunk, sizeof(chunk));
            if (n < 0 && (errno == EAGAIN || errno == EINTR)) {
                continue;
            }
            if (n <= 0) {
                lost();
                continue;
            }
            for (ssize_t i = 0; i < n; ++i) {
                const char c = chunk[i];
                if (c == '\n' || c == '\r') {
                    if (!buffer.empty()) {
                        on_line(buffer);
                        buffer.clear();
                    }
                } else if (buffer.size() < MAX_LINE) {
                    buffer += c;
                } else {
                    buffer.clear();
                }
            }
        }
    }

    const std::string device;
    std::mutex fd_mutex;
    int fd = -1;
    std::thread reader;
};

}  // namespace

std::shared_ptr<Olimexino> make_olimexino(const YAML::Node& config) {
    if (!config["device"]) {
        throw std::runtime_error(
            "olimexino: missing 'device' (e.g. /dev/serial/by-id/...)");
    }
    auto board = std::make_shared<SerialOlimexino>(
        config["device"].as<std::string>());
    board->start();
    return board;
}
