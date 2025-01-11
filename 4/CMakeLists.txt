cmake_minimum_required(VERSION 3.10)
project(4)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(USE_SIMULATION "Use simulation mode" OFF)
   
if(USE_SIMULATION)
    add_definitions(-DUSE_SIMULATION)
endif()

find_program(CLANG_TIDY clang-tidy)
if(CLANG_TIDY)
    set_target_properties(4 PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY};--warnings-as-errors=*")
    message(STATUS "Clang-Tidy enabled")
endif()

find_program(CLANG_FORMAT clang-format)
if(CLANG_FORMAT)
    message(STATUS "Clang-Format enabled")
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    add_custom_target(format
        COMMAND ${CLANG_FORMAT} -i ${CMAKE_SOURCE_DIR}/*.cpp ${CMAKE_SOURCE_DIR}/*.h
        DEPENDS ${CMAKE_SOURCE_DIR}/*.cpp ${CMAKE_SOURCE_DIR}/*.h
    )
    
    add_custom_target(check_format
        COMMAND ${CLANG_FORMAT} -n ${CMAKE_SOURCE_DIR}/*.cpp ${CMAKE_SOURCE_DIR}/*.h
        DEPENDS ${CMAKE_SOURCE_DIR}/*.cpp ${CMAKE_SOURCE_DIR}/*.h
    )
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
