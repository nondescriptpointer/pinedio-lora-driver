project(communicator)
cmake_minimum_required(VERSION 3.21)

add_library(communicator
    src/Communicator.cpp
    include/PineDio/LoRa/Communicator.h)

target_link_libraries(communicator
    pinedio-lora-driver
    pthread
)

target_include_directories(communicator PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/apps/communicator/include>
    )