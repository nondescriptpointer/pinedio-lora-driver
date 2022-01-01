project(usb-communicator)
cmake_minimum_required(VERSION 3.21)

add_executable(usb-communicator
    main.cpp)

target_link_libraries(usb-communicator
    communicator
    pinedio-lora-driver-usb
    )