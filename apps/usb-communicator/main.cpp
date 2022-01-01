#include <PineDio/LoRa/Communicator.h>
#include <iostream>
#include "PineDio/LoRa/UsbAdapter.h"
#include "PineDio/LoRa/Exceptions.h"

int main() {
  try {
    PineDio::LoRa::UsbAdapter usbAdapter;
    PineDio::LoRa::PinedioLoraRadio radio(usbAdapter);
    PineDio::LoRa::Communicator communicator(radio);
    communicator.Run();
  } catch(const PineDio::LoRa::InitializationException& ex) {
    std::cerr << "Initialization error : " << ex.what() << std::endl;
  }

  return 0;
}