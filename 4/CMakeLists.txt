cmake_minimum_required(VERSION 3.10)
project(4)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(USE_SIMULATION "Use simulation mode" OFF)

if (USE_SIMULATION)
    add_definitions(-DUSE_SIMULATION)
endif()

add_executable(4 
    main.cpp
    serial_port.cpp
    temperature_sensor.cpp
    logger.cpp
)

include_directories(.)

set_target_properties(4 PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON)


cmake_minimum_required(VERSION 3.10)
project(4)
