# Client Pi
C++ Client software that runs on the Raspberry Pi on the Beneficiation and Excavation systems for the Coyote3 robot.

See https://github.com/esa-esric-bremen/.github/ for a system overview.

<img src="https://avatars.githubusercontent.com/u/277213809?s=400&u=25dd27e12b5732cb78306bf27b43139214900f58" style="width:200px;">

[![Pioneers of Sustainable Lunar Missions: Team BREMEN Wins the International Space Resources Challenge](https://img.youtube.com/vi/bCoAqg_29Lo/0.jpg)](https://www.youtube.com/watch?v=bCoAqg_29Lo)

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