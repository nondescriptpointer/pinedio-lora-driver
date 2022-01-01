#include <PineDio/LoRa/UsbAdapter.h>
#include <iostream>
#include <unistd.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

using namespace PineDio::LoRa;

UsbAdapter::UsbAdapter() {
  std::cout << "[UsbAdapter] ctor()" << std::endl;

  // TODO remove code duplication for exporting GPIO
  if(access("/sys/class/gpio/ini", F_OK) != 0 ) {
    int fd;
    if ((fd = open("/sys/class/gpio/export", O_WRONLY)) == -1) {
      perror("Error while opening GPIO export file");
      // TODO ERROR
    }
    if (write(fd, "240", 3) == -1) { // TODO how to find the GPIO number programatically?
      perror("Error while exporting GPIO \"ini\"");
      // TODO ERROR
    }
    close(fd);
  }

  if(access("/sys/class/gpio/slct", F_OK) != 0 ) {
    int fd;
    if ((fd = open("/sys/class/gpio/export", O_WRONLY)) == -1) {
      perror("Error while opening GPIO export file");
      // TODO ERROR
    }
    if (write(fd, "244", 3) == -1) { // TODO how to find the GPIO number programatically?
      perror("Error while exporting GPIO \"sclt\"");
      // TODO ERROR
    }
    close(fd);
  }

  handle = open("/dev/spidev0.0", O_RDWR);
  if(handle == -1) {
    // TODO error
    printf("SPI IOCTL error %s\n", strerror(errno));
  }

  uint8_t mmode = SPI_MODE_0;
  uint8_t lsb = 0;
  uint8_t bitsperword = 8;
  ioctl(handle, SPI_IOC_RD_BITS_PER_WORD, &bitsperword);
  ioctl(handle, SPI_IOC_WR_MODE, &mmode);
  ioctl(handle, SPI_IOC_WR_LSB_FIRST, &lsb);
}

uint8_t UsbAdapter::HalGpioRead(SX126x::GpioPinFunction_t func) {
  if(func != GpioPinFunction_t::GPIO_PIN_BUSY) {
    std::cout << "ERROR" << std::endl;
    throw;
  }

  int  fd;
  if ((fd = open("/sys/class/gpio/slct/value", O_RDWR)) == -1)   {
    perror("Error while opening GPIO \"busy\"");
    return -1;
  }

  char buf;
  if (read(fd, &buf, 1) == -1) {
    perror("Error while reading GPIO \"busy\"");
    return -1;
  }
  close(fd);

  int value = (buf == '0') ? 0 : 1;

  std::this_thread::sleep_for(std::chrono::milliseconds{1}); // Why do I need this sleep()?
  return value;
}

void UsbAdapter::HalGpioWrite(SX126x::GpioPinFunction_t func, uint8_t value) {
  if(func != GpioPinFunction_t::GPIO_PIN_RESET) {
    std::cout << "ERROR" << std::endl;
    throw;
  }

  int  fd;

  if ((fd = open("/sys/class/gpio/ini/value", O_RDWR)) == -1)   {
    perror("Error while opening GPIO \"reset\"");
  }

  if (write(fd, value ? "1" : "0", 1) == -1) {
    perror ("Error while writing GPIO \"reset\"");
  }
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

  int status = ioctl (handle, SPI_IOC_MESSAGE(1), &spi_trans);
}

