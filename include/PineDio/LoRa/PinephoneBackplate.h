#pragma once
#include <PineDio/LoRa/PinedioLoraRadio.h>
#include <memory>

namespace PineDio::LoRa {

class PinephoneBackplate : public SX126x {
public:
  explicit PinephoneBackplate(const std::string& i2cFilename);
  void Initialize();

private:
  uint8_t HalGpioRead(GpioPinFunction_t func) override;
  void HalGpioWrite(GpioPinFunction_t func, uint8_t value) override;
  void HalSpiTransfer(uint8_t *buffer_in, const uint8_t *buffer_out, uint16_t size) override;

  int handle;
  void RxDone();
  void SyncI2CBuffer();
};

}
