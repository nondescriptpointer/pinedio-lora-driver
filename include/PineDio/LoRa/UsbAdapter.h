#pragma once
#include <PineDio/LoRa/PinedioLoraRadio.h>
#include <memory>
#include <linux/gpio.h>

namespace PineDio::LoRa {

class UsbAdapter : public SX126x {
public:
  UsbAdapter();
  ~UsbAdapter() override;

private:
  uint8_t HalGpioRead(GpioPinFunction_t func) override;
  void HalGpioWrite(GpioPinFunction_t func, uint8_t value) override;
  void HalSpiTransfer(uint8_t *buffer_in, const uint8_t *buffer_out, uint16_t size) override;
  void OpenSpi();
  void OpenGpio();

  int handle;
  struct gpiohandle_request resetGpio;
  struct gpiohandle_request busyGpio;

  void RxDone();
};

}
