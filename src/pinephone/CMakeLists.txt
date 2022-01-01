project(pinedio-lora-driver-pinephone CXX)

set(PINEDIO_LORA_DRIVER_SOURCE_FILES
    PinephoneBackplate.cpp
)

set(PINEDIO_LORA_DRIVER_PUBLIC_HEADERS
    ../../include/PineDio/LoRa/PinedioLoraRadio.h
    ../../include/PineDio/LoRa/PinephoneBackplate.h
    )

add_library(pinedio-lora-driver-pinephone
    ${PINEDIO_LORA_DRIVER_PUBLIC_HEADERS}
    ${PINEDIO_LORA_DRIVER_SOURCE_FILES}
    )

target_link_libraries(pinedio-lora-driver-pinephone PUBLIC
    pinedio-lora-driver
    )

target_include_directories(pinedio-lora-driver-pinephone PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/>
)