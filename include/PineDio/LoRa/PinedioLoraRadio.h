#pragma once

#include <string>
#include <memory>
#include <sx126x_driver/SX126x.hpp>

namespace PineDio {
namespace LoRa {
class PinedioLoraRadio {
public:
  explicit PinedioLoraRadio(SX126x& radio);

  virtual void Initialize();
  virtual void Send(std::vector<uint8_t> data);
  virtual std::vector<uint8_t> Receive(std::chrono::milliseconds timeout);
private:
  SX126x& radio;
  bool dataReceived {false};
  std::vector<uint8_t> receivedBuffer;

  bool dataToSend {false};
  std::vector<uint8_t> transmitBuffer;

  void OnDataReceived();
};
}
}