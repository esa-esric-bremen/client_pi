# Client Pi
C++ Client software that runs on the Raspberry Pi on the Beneficiation and Excavation systems for the Coyote3 robot.

This Client application for RRC (Controlled Robot) was developed at the [Robotics Innovation Center](http://robotik.dfki-bremen.de/en/startpage.html) of the [German Research Center for Artificial Intelligence (DFKI)](http://www.dfki.de) in Bremen.

See https://github.com/esa-esric-bremen/.github/ for more details, including a system overview.
<table style="border: 0px;">
    <tr><td>
        <img src="https://avatars.githubusercontent.com/u/277213809?s=400&u=25dd27e12b5732cb78306bf27b43139214900f58" style="width:300px;">
    </td><td>
        <a href="https://www.youtube.com/watch?v=bCoAqg_29Lo" alt="Pioneers of Sustainable Lunar Missions: Team BREMEN Wins the International Space Resources Challenge"><img src="https://img.youtube.com/vi/bCoAqg_29Lo/0.jpg"></a>
    </td></tr>
    <tr>
        <td>Team Logo</td>
        <td>Youtube Video: Pioneers of Sustainable Lunar Missions: Team BREMEN Wins the International Space Resources Challenge</td>
    </tr>
</table>

## Motivation
This is code for the **winning** team B.R.E.M.E.N. of the [ESA ESRIC Space Resource Challenge](https://src.esa.int) as reference implementation for [Robot Remote Control](https://github.com/dfki-ric/robot_remote_control) and general information for the public.

### Installation
Either use the Dev-container for local devleopment or for deployment on a Raspberry Pi install it the requirements using apt and compile robot-remote-control as well as the latest version of WiringPi:

```bash
$ sudo apt-get install -y --no-install-recommends \
    build-essential \
    git \
    cmake \
    pkg-config \
    libzmq3-dev \
    libprotobuf-dev \
    protobuf-compiler \
    libreadline-dev \
    libncurses-dev \
    libopencv-dev \
    ca-certificates \
    gettext-base \
    fakeroot \
    libyaml-cpp-dev
# install third-party packages
$ sudo mkdir -p /app/client_pi/thirdparty
$ sudo chown $USER:$GROUP -R /app/client_pi/
$ cd /app/client_pi/thirdparty
$ git clone https://github.com/dfki-ric/robot_remote_control.git \
    && cd robot_remote_control \
    && mkdir build \
    && cd build \
    && cmake .. \
    && make -j$(nproc) \
    && sudo make install \
    && sudo ldconfig

$ git clone https://github.com/WiringPi/WiringPi.git \
    && cd WiringPi \
    && ./build debian \
    && sudo dpkg -i ./debian-template/wiringpi_*_arm64.deb
```

### Dependencies
This client (ControlledRobot) requires [Robot-Remote-Control](https://github.com/dfki-ric/robot_remote_control) which depends on libzmq and protobuf.

All connected devices to the Raspberry Pi are controlled by [WiringPi](https://github.com/WiringPi/WiringPi).

### Getting Started
Deploy to the rasperry pi and build there or for simulation open this folder in code and build&run the devcontainer (also take a look at the `compose.yml`-file).

### OLIMEXINO motor controller (CAN motors)
The CubeMars AK80-6 and the SitoDrive TA70-100 are on CAN, driven by an OLIMEXINO-STM32F3
(firmware in `firmware/olimexino`) that is connected to the Pi by USB serial. Configure it in
the robot config:

```yaml
olimexino:
  device: /dev/serial/by-id/usb-...   # or /dev/ttyACM0
```

The user running `client_pi` needs access to the port (`sudo usermod -aG dialout <user>`).

| driver | config | behavior |
|---|---|---|
| `olimexino_ak80` (action) | `range` in ERPM (max +/-100000) | sends `ak <erpm>`; stop: 0 ERPM, init: 0 mA (no torque) |
| `olimexino_ta70` (action) | `range` in deg (0..45), `speed_rpm`, `accel_rpm_s`, `current_ma` | keeps the latest target and resends it until the board confirms it (the firmware ignores targets during a move); init enters Profile Position Mode, never done automatically because it can release the brake; stop aborts (brake locks, init needed again) |
| `olimexino` (sensor) | `channel`: a telemetry name (`AK_ActualERPM`, `AK_CurrentA`, `TA_ActualDeg`, ...) or `link` | `link` is 1 while telemetry arrives, a missing channel is not reported |

Safety: the client sends `ping` every 250 ms and `stop all` when the ground station
connection is lost. With the watchdog firmware the board stops everything by itself when
the Pi stops talking (crash, cable). The connection to the board reconnects on its own.

In SIMULATED builds a simulated board with the same command rules is used.

### Simulation
Built with `-DSIMULATED=ON` (default, see `Dockerfile-dev`), the client runs without additional hardware:
- `client_pi_main <config.yaml>` uses the dummy drivers in `src/driver/dummy.cpp`, values come from the excavation model in `src/driver/sim_model.cpp`. Actuators are found by the commands of their actions (`excavator_speed`, `screw_speed`, `linear_motor_1`, `led_1`), sensors react to what the operator does.
- `coyote_sim <config_coyote.yaml>` simulates the Coyote 3 rover (twist commands, LEDs, pose/velocity telemetry), so the ground station can be tested completely.

`esa-src-docker-env` runs both as docker containers (`./run_sim.bash`).

### Hardwarde Notes
Optionally, to improve performance, increase the I2C baudrate from the default of 100KHz to 400KHz by altering `/boot/config.txt` to include:

`dtparam=i2c_arm=on,i2c_baudrate=400000`

### Raspberry Pi setup
1. Check out the repository on the Raspberry Pi using git.
1. Install all dependencies (see Installation-section above)
1. (re-)compile the code (see `./rpi_rebuild.bash`):
```bash
cd /app/client_pi/
mkdir -p build
cd build/
cmake -DSIMULATED=OFF ..
make
mv ./client_pi_main ..
cd ..
```
4. Copy the client_pi.service file to automatically start when the raspberry pi boots up:
```bash
sudo cp local_files/systemd/system/client_pi.service  /etc/systemd/system/client_pi.service
```

You can then start the client using
```bash
$ /app/clinet_pi/client_pi
```

## License
The base is released as BSD-3.

The INA228 driver is licensed using the MIT Licens, its repository can be found here: https://github.com/RobTillaart/INA228

The code for the HX711 weight cells was originally released in python as https://github.com/tatobari/hx711py/blob/master/hx711.py under the Apache License Version 2.0 and has been translated into C++ with some help of Google Gemini.

## Maintainer / Authors / Contributers
Andreas Bresser, Andreas.Bresser@dfki.de

Copyright 2025-2026, DFKI GmbH / Robotics Innovation Center
