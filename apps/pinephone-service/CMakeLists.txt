project(pinephone-service)
cmake_minimum_required(VERSION 3.21)

add_executable(pinephone-service src/main.cpp)

target_link_libraries(pinephone-service
    pinedio-lora-driver-pinephone
    )