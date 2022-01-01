project(pinephone-communicator)
cmake_minimum_required(VERSION 3.21)

add_executable(pinephone-communicator
    main.cpp)

target_link_libraries(pinephone-communicator
    communicator
    pinedio-lora-driver-pinephone
    )