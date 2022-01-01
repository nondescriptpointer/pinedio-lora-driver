cmake_minimum_required(VERSION 3.17)
project(sx126x_driver)

set(CMAKE_CXX_STANDARD 14)

add_library(sx126x_driver sx126x_driver/SX126x.cpp sx126x_driver/SX126x.hpp)

target_include_directories(sx126x_driver PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs>
    )