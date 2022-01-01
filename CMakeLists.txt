project(pinedio-lora-driver-project)
cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 20)

option(BUILD_FOR_PINEPHONE "Build PinePhone support" FALSE)
option(BUILD_FOR_USB "Build support for the USB adapter" FALSE)

add_subdirectory(src)
add_subdirectory(libs)
add_subdirectory(apps)

