#pragma once

#include "PineDio/LoRa/PinedioLoraRadio.h"
namespace PineDio::LoRa {
class PinedioLoraRadio;
class Communicator {
public:
  explicit Communicator(PineDio::LoRa::PinedioLoraRadio &radio);

  void Run();

private:
  PineDio::LoRa::PinedioLoraRadio &radio;
  std::atomic<bool> running{false};
  std::unique_ptr<std::thread> receiveTask;
  void Receive();
};
}


