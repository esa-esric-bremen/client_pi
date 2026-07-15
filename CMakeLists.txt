cmake_minimum_required(VERSION 3.10)
project(client_pi)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(SIMULATED "Use SIMULATED flag for development" ON)

if(SIMULATED)
    message(STATUS "Build mode: SIMULATED")
else()
    message(STATUS "Build mode: Real hardware")
endif()

find_package(yaml-cpp REQUIRED)
find_package(Protobuf REQUIRED)

find_package(robot_remote_control REQUIRED COMPONENTS
    controlled_robot
    types
    update_thread
    transport_zmq
)

if(NOT SIMULATED)
    find_library(WIRINGPI_LIBRARY NAMES wiringPi)
endif()

add_library(DummyDriver INTERFACE)
target_sources(DummyDriver INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/driver/dummy.cpp)

add_library(RaspberryPiDriver INTERFACE)
target_sources(RaspberryPiDriver INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/driver/gpio.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/driver/hx711.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/driver/ina228.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/driver/l298n.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/driver/ads1x15.cpp
)
target_link_libraries(RaspberryPiDriver INTERFACE wiringPi)

if(NOT SIMULATED)
    target_link_libraries(RaspberryPiDriver INTERFACE ${WIRINGPI_LIBRARY})
endif()

set(APP_SOURCES
    src/main.cpp
    src/actions/motor_driver.cpp
    src/actions/gpio_driver.cpp
    src/automation/automation.cpp
    src/automation/step.cpp
    src/automation/step_action.cpp
    src/automation/task.cpp
    src/sensors/cpu_temp.cpp
    src/sensors/power_monitor.cpp
    src/sensors/sensor.cpp
    src/sensors/vcgencmd.cpp
    src/sensors/weight_sensor.cpp
)

add_executable(client_pi_main ${APP_SOURCES})

if(SIMULATED)
    target_compile_definitions(client_pi_main PRIVATE SIMULATED)
    set(DRIVER_LIB DummyDriver)
else()
    set(DRIVER_LIB RaspberryPiDriver)
endif()

target_link_libraries(client_pi_main PRIVATE
    robot_remote_control-controlled_robot
    robot_remote_control-types
    robot_remote_control-update_thread
    robot_remote_control-transport_zmq
    protobuf
    yaml-cpp

    ${DRIVER_LIB}
)

