project(pinedio-lora-driver CXX)

add_library(pinedio-lora-driver
    ../include/PineDio/LoRa/PinedioLoraRadio.h
    ../include/PineDio/LoRa/Exceptions.h
    PinedioLoraRadio.cpp
    )

target_link_libraries(pinedio-lora-driver PUBLIC
    sx126x_driver
    )

target_include_directories(pinedio-lora-driver PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/>
    )


set(AT_LEAST_ONE_BUILD_ENABLED FALSE)
if(${BUILD_FOR_PINEPHONE})
    add_subdirectory(pinephone)
    set(AT_LEAST_ONE_BUILD_ENABLED TRUE)
endif()

if(${BUILD_FOR_USB})
    add_subdirectory(usb-adapter)
    set(AT_LEAST_ONE_BUILD_ENABLED TRUE)
endif()

if(NOT ${AT_LEAST_ONE_BUILD_ENABLED})
    message(FATAL_ERROR "Please select at least one build (-DBUILD_FOR_PINEPHONE or -DBUILD_FOR_USB)")
endif()