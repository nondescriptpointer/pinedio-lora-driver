project(pinedio-lora-driver-usb CXX)

set(PINEDIO_LORA_DRIVER_SOURCE_FILES
    UsbAdapter.cpp
)

set(PINEDIO_LORA_DRIVER_PUBLIC_HEADERS
    ../../include/PineDio/LoRa/PinedioLoraRadio.h
    ../../include/PineDio/LoRa/UsbAdapter.h
    )

add_library(pinedio-lora-driver-usb
    ${PINEDIO_LORA_DRIVER_PUBLIC_HEADERS}
    ${PINEDIO_LORA_DRIVER_SOURCE_FILES}
    )

target_link_libraries(pinedio-lora-driver-usb
    sx126x_driver
    )

target_include_directories(pinedio-lora-driver-usb PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/>
)