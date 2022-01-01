#include <PineDio/LoRa/PinephoneBackplate.h>
#include <PineDio/LoRa/Exceptions.h>
#include <iostream>

#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
using namespace PineDio::LoRa;

PinephoneBackplate::PinephoneBackplate(const std::string &i2cFilename) {
  std::cout << "[PinephoneBackplate] ctor()" << std::endl;

  if ((handle = open(i2cFilename.c_str(), O_RDWR)) < 0) {
    /* ERROR HANDLING: you can check errno to see what went wrong */
    perror("Failed to open the i2c bus");
    exit(1);
  }

  int addr = 0x28;          // The I2C address of the ADC
  if (ioctl(handle, I2C_SLAVE, addr) < 0) {
    printf("Failed to acquire bus access and/or talk to slave.\n");
    /* ERROR HANDLING; you can check errno to see what went wrong */
    exit(1);
  }
}

void PinephoneBackplate::Initialize() {
  SyncI2CBuffer();
  /*
    uint8_t bufferWrite = 4;
    if (write(handle, &bufferWrite,1) != 1) {
      printf("Failed to write to the i2c bus.\n");
      printf("\n\n");
      exit(1);
    }
  */
}


uint8_t PinephoneBackplate::HalGpioRead(SX126x::GpioPinFunction_t func) {
  usleep(10000);
  return 0;
}

void PinephoneBackplate::HalGpioWrite(SX126x::GpioPinFunction_t func, uint8_t value) {
  usleep(10000);
}

void PinephoneBackplate::HalSpiTransfer(uint8_t *buffer_in,
                                              const uint8_t *buffer_out,
                                              uint16_t size) {
  //std::cout << "Start transfer " << size +1<< " bytes\n\t";
  uint8_t bufferWrite[size+1];
  uint8_t bufferRead[size+1];
  bufferWrite[0] = 1; // CMD TRANSMIT
  std::memcpy(bufferWrite+1, buffer_out, size);

  //for(int i = 0; i< size+1; i++) {
  //  printf("%x ", bufferWrite[i]);
 // }
 // std::cout << std::endl;

  if (write(handle, bufferWrite,size+1) != size+1) {
    /* ERROR HANDLING: i2c transaction failed */
    printf("Failed to write to the i2c bus.\n");
    printf("\n\n");
    return;
  }
  //std::cout << "\t Write OK\n";

  //std::cout << "Reading " << size << " bytes from the bus \n";
  for(int i = 0; i < size; i++) {
    if (read(handle, buffer_in + i, 1) != 1) {
      /* ERROR HANDLING: i2c transaction failed */
      printf("Failed to read from the i2c bus.\n");
      printf("\n\n");
      return;
    }
  }
}

void PinephoneBackplate::SyncI2CBuffer() {
  std::cout << "[PinephoneBackplate] Sync buffer..." << std::endl;

  std::cout << "\tInit LoRa module..." << std::endl;
  // Set radio to idle
  uint8_t dd[3] = {0x01, 0x80, 0x00};
  write(handle, &dd, 3);
  usleep(1000);

  // Set RX/TX index to 0
  uint8_t d[4] = {0x01, 0x8f, 0x00, 0x00};
  write(handle, &d, 4);
  usleep(1000);

  std::cout << "\tSend verification data..." << std::endl;
  uint8_t cmd1[] = {0x01, 0x0E, 0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0xAA, 0x55, 0x00, 0xFF};
  write(handle, cmd1, 12);
  usleep(1000);

  uint8_t cmd2[] = {0x01, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  usleep(1000);
  write(handle, cmd2, 13);
  usleep(1000);

  std::cout << "\tSync'ing..." << std::endl;
  bool done = false;
  bool sequenceStarted = false;
  size_t sequenceIndex = 0;
  std::array<uint8_t, 9> sequence = {0x10, 0x20, 0x30, 0x40, 0x50, 0xAA, 0x55, 0x00, 0xFF};
  int count = 0;
  while(!done && count < 256) {
    int d = 0;
    read(handle, &d, 1);

    if(!sequenceStarted) {
      for(int i = 0; i < 9; i++) {
        if(d == sequence[i]){
          sequenceStarted = true;
          sequenceIndex = i;
        }
      }
    } else {
      if(d == sequence[sequenceIndex+1]) {
        sequenceIndex++;
        if(sequenceIndex == 8) {
          done = true;
          std::cout << "\tSync done after " << count << " bytes!"  << std::endl;

        }
      } else {
        sequenceStarted = false;
      }
    }
    count ++;
  }
  if(count >= 256) {
    throw InitializationException("Internal buffer synchronization failed");
  }

}
