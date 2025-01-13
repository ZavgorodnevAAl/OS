cmake_minimum_required(VERSION 3.10)
project(5)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(USE_SIMULATION "Use simulation mode" OFF)

if (USE_SIMULATION)
    add_definitions(-DUSE_SIMULATION)
endif()

add_executable(5 
    main.cpp
    serial_port.cpp
    temperature_sensor.cpp
    logger.cpp
)

target_link_libraries(5 pthread sqlite3)

include_directories(.)

set_target_properties(5 PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

cmake_minimum_required(VERSION 3.10)
project(5)
