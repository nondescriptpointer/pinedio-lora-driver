#include <PineDio/LoRa/PinedioLoraRadio.h>
#include <PineDio/LoRa/PinephoneBackplate.h>
#include <PineDio/LoRa/Communicator.h>
#include <PineDio/LoRa/Exceptions.h>
#include <iostream>

int main() {
  try {
    PineDio::LoRa::PinephoneBackplate pinephoneBackplate("/dev/i2c-2");
    pinephoneBackplate.Initialize();
    PineDio::LoRa::PinedioLoraRadio radio(pinephoneBackplate);
    PineDio::LoRa::Communicator communicator(radio);
    communicator.Run();
  } catch(const PineDio::LoRa::InitializationException& ex) {
    std::cerr << "Initialization error : " << ex.what() << std::endl;
  }

  return 0;
}
