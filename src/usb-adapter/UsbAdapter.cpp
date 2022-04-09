#include <PineDio/LoRa/UsbAdapter.h>
#include <iostream>
#include <unistd.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <dirent.h>
#include <linux/gpio.h>

using namespace PineDio::LoRa;

namespace {
const char *spiDriverOverrideFilePath ="/sys/class/spi_master/spi0/spi0.0/driver_override";
const char *spidevOverride = "spidev";

const char *spiBindFilePath = "/sys/bus/spi/drivers/spidev/bind";
const char *spiUnbindFilePath = "/sys/bus/spi/drivers/spidev/unbind";
const char *spiBind = "spi0.0";

std::vector<std::string> GetChipChipNames() {
  auto gpioFilenameFilter = [](const struct dirent *entry) -> int {
    std::string filename = entry->d_name;
    return filename.rfind("gpiochip", 0) == 0;
  };
  static const char *devString = "/dev/";

  std::vector<std::string> filenames;
  struct dirent **entries;
  auto nb = scandir(devString, &entries, gpioFilenameFilter, alphasort);
  for (int i = 0; i < nb; i++) {
    std::string name = std::string{devString} + std::string{entries[i]->d_name};
    filenames.push_back(name);
  }
  return filenames;
}

bool GetChipInfo(const std::string chip, struct gpiochip_info &info) {
  auto fd = open(chip.c_str(), O_RDWR);
  auto res = ioctl(fd, GPIO_GET_CHIPINFO_IOCTL, &info);
  return res == 0;
}

std::string GetCh341ChipFilename() {
  static const char *ch341ChipLabel = "ch341";
  static const size_t ch341NbLines = 16;

  auto chips = GetChipChipNames();
  for (auto &chip : chips) {
    struct gpiochip_info info;
    if (GetChipInfo(chip, info)) {
      if (std::string{info.label} == std::string{ch341ChipLabel} &&
          info.lines == ch341NbLines)
        return chip;
    }
  }
  throw std::runtime_error("Cannot find a GPIO chip corresponding to the CH341");
}

int GetGpioLineIndex(const std::string &chip, const char *name) {
  struct gpiochip_info info;
  auto fd = open(chip.c_str(), O_RDWR);
  auto res = ioctl(fd, GPIO_GET_CHIPINFO_IOCTL, &info);
  if(res != 0)
    throw std::runtime_error("IOCTL error (GPIO_GET_CHIPINFO_IOCTL failed)");

  for (int i = 0; i < info.lines; i++) {
    struct gpioline_info infoLine;
    infoLine.line_offset = i;
    res = ioctl(fd, GPIO_GET_LINEINFO_IOCTL, &infoLine);
    if(res != 0)
      throw std::runtime_error("IOCTL error (GPIO_GET_LINEINFO_IOCTL failed)");

    if (strcmp(name, infoLine.name) == 0)
      return i;
  }
  throw std::runtime_error("Cannot find GPIO line");
}
}

UsbAdapter::UsbAdapter() {
  std::cout << "[UsbAdapter] ctor()" << std::endl;
  OpenGpio();
  OpenSpi();
}

UsbAdapter::~UsbAdapter() {
  close(handle);
  auto fd = open(spiUnbindFilePath, O_WRONLY);
  write(fd, spiBind, strlen(spiBind));

  close(fd);
}

uint8_t UsbAdapter::HalGpioRead(SX126x::GpioPinFunction_t func) {
  if(func != GpioPinFunction_t::GPIO_PIN_BUSY)
    throw std::runtime_error("Invalid call to HalGpioRead()");

  struct gpiohandle_data data;
  memset(&data, 0, sizeof(data));
  auto res = ioctl(busyGpio.fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data);
  if(res != 0)
    throw std::runtime_error("IOCTL error (GPIOHANDLE_GET_LINE_VALUES_IOCTL failed)");

  int value = data.values[0];

  std::this_thread::sleep_for(std::chrono::milliseconds{1}); // Why do I need this sleep()?
  return value;
}

void UsbAdapter::HalGpioWrite(SX126x::GpioPinFunction_t func, uint8_t value) {
  if(func != GpioPinFunction_t::GPIO_PIN_RESET)
    throw std::runtime_error("Invalid call to HalGpioWrite()");

  struct gpiohandle_data data;
  data.values[0] = value;
  auto res = ioctl(resetGpio.fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
  if(res != 0)
    throw std::runtime_error("IOCTL error (GPIOHANDLE_SET_LINE_VALUES_IOCTL failed)");
}

void UsbAdapter::HalSpiTransfer(uint8_t *buffer_in, const uint8_t *buffer_out, uint16_t size) {
  const uint8_t *mosi = buffer_out; // output data
  uint8_t *miso = buffer_in; // input data

  struct spi_ioc_transfer spi_trans;
  memset(&spi_trans, 0, sizeof(spi_trans));

  spi_trans.tx_buf = (unsigned long) mosi;
  spi_trans.rx_buf = (unsigned long) miso;
  spi_trans.cs_change = true;
  spi_trans.len = size;

  int res = ioctl(handle, SPI_IOC_MESSAGE(1), &spi_trans);
  if(res < 0)
    throw std::runtime_error("IOCTL error (SPI_IOC_MESSAGE failed)");
}

void UsbAdapter::OpenSpi() {
  auto fd = open(spiDriverOverrideFilePath, O_WRONLY);
  if(fd == -1)
    throw std::runtime_error("Cannot open SPI device (driver_override open error)");

  auto res = write(fd, spidevOverride, strlen(spidevOverride));
  if(res <= 0)
    throw std::runtime_error("Cannot open SPI device (driver_override write error)");

  close(fd);

  fd = open(spiBindFilePath, O_WRONLY);
  if(fd == -1)
    throw std::runtime_error("Cannot open SPI device (bind open error)");

  res = write(fd, spiBind, strlen(spiBind));
  if(res <= 0)
    throw std::runtime_error("Cannot open SPI device (bind write error)");

  close(fd);

  handle = open("/dev/spidev0.0", O_RDWR);
  if(handle == -1)
    throw std::runtime_error("Cannot open SPI device (spidev open error)");

  uint8_t mmode = SPI_MODE_0;
  uint8_t lsb = 0;
  uint8_t bitsperword = 8;
  ioctl(handle, SPI_IOC_RD_BITS_PER_WORD, &bitsperword);
  ioctl(handle, SPI_IOC_WR_MODE, &mmode);
  ioctl(handle, SPI_IOC_WR_LSB_FIRST, &lsb);
}

void UsbAdapter::OpenGpio() {
  auto chipName = GetCh341ChipFilename();
  std::cout << "CH341 detected at " << chipName << std::endl;

  auto resetlineIndex = GetGpioLineIndex(chipName, "ini");
  std::cout << "GPIO RESET detected at index " << resetlineIndex << std::endl;

  struct gpiochip_info info;
  auto fd = open(chipName.c_str(), O_RDWR);

  resetGpio.flags = GPIOHANDLE_REQUEST_OUTPUT;
  resetGpio.lines = 1;
  resetGpio.lineoffsets[0] = resetlineIndex;
  resetGpio.default_values[0] = 0;
  strcpy(resetGpio.consumer_label, "LoRa SPI driver");
  ioctl(fd, GPIO_GET_LINEHANDLE_IOCTL, &resetGpio);

  auto busylineIndex = GetGpioLineIndex(chipName, "slct");
  std::cout << "GPIO BUSY detected at index " << busylineIndex << std::endl;

  busyGpio.flags = GPIOHANDLE_REQUEST_INPUT;
  busyGpio.lines = 1;
  busyGpio.lineoffsets[0] = busylineIndex;
  busyGpio.default_values[0] = 0;
  strcpy(busyGpio.consumer_label, "LoRa SPI driver");
  ioctl(fd, GPIO_GET_LINEHANDLE_IOCTL, &busyGpio);
}
