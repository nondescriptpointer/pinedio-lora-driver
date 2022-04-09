#include <PineDio/LoRa/Communicator.h>
#include <iostream>
#include "PineDio/LoRa/UsbAdapter.h"
#include "PineDio/LoRa/Exceptions.h"
#include <signal.h>

PineDio::LoRa::UsbAdapter usbAdapter;
PineDio::LoRa::PinedioLoraRadio radio(usbAdapter);
PineDio::LoRa::Communicator communicator(radio);

void sig_handler(int signum) {
  std::cout << "Press ENTER to exit..." << std::endl;
  communicator.Stop();
}

int main() {
  signal(SIGINT, sig_handler);

  try {
    communicator.Run();
  } catch(const PineDio::LoRa::InitializationException& ex) {
    std::cerr << "Initialization error : " << ex.what() << std::endl;
  }

  return 0;
}