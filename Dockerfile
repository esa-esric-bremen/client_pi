FROM ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
  --mount=type=cache,target=/var/lib/apt,sharing=locked \
  apt-get update -qq \
    && apt-get install -y --no-install-recommends \
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

# libudt-dev does not exist for ARM64

RUN mkdir -p /app/client_pi/thirdparty
WORKDIR /app/client_pi/thirdparty

# basics and network libraries
RUN git clone https://github.com/dfki-ric/robot_remote_control.git \
    && cd robot_remote_control \
    && mkdir build \
    && cd build \
    && cmake .. \
    && make -j$(nproc) \
    && make install \
    && ldconfig

# device drivers
RUN git clone https://github.com/WiringPi/WiringPi.git \
    && cd WiringPi \
    && ./build debian \
    && dpkg -i ./debian-template/wiringpi_*_arm64.deb

WORKDIR /app/client_pi
