#include <PineDio/LoRa/PinedioLoraRadio.h>
#include <sx126x_driver/SX126x.hpp>
#include <iostream>
#include <unistd.h>

using namespace PineDio::LoRa;

PinedioLoraRadio::PinedioLoraRadio(SX126x &radio) : radio{radio} {}

void PinedioLoraRadio::Initialize() {
  std::cout << "[PinedioLoraRadio] Initialize()" << std::endl;

  radio.callbacks.rxDone = [this](){OnDataReceived();};
  radio.callbacks.txDone = [this](){
    std::cout << "TX DONE" << std::endl;
    usleep(10000);
    radio.SetRx(0xffffffff);
    usleep(10000);
  };

  radio.SetDeviceType(SX126x::SX1262);
  radio.Init();
  radio.SetDio2AsRfSwitchCtrl(true);
  radio.SetStandby(SX126x::RadioStandbyModes_t::STDBY_RC);
  radio.SetRegulatorMode(SX126x::RadioRegulatorMode_t::USE_DCDC);
  radio.SetBufferBaseAddresses(0,127);

  radio.SetTxParams(22, SX126x::RadioRampTimes_t::RADIO_RAMP_3400_US);
  radio.SetDioIrqParams(0xffff, 0x0001, 0x0000, 0x0000);
  radio.SetRfFrequency(868000000);

  radio.SetPacketType(SX126x::RadioPacketTypes_t::PACKET_TYPE_LORA);

  radio.SetStopRxTimerOnPreambleDetect(false);

  SX126x::ModulationParams_t modulationParams;
  modulationParams.PacketType = SX126x::RadioPacketTypes_t::PACKET_TYPE_LORA;
  modulationParams.Params.LoRa.LowDatarateOptimize = 0;
  modulationParams.Params.LoRa.Bandwidth = SX126x::RadioLoRaBandwidths_t::LORA_BW_500;
  modulationParams.Params.LoRa.CodingRate = SX126x::RadioLoRaCodingRates_t::LORA_CR_4_5;
  modulationParams.Params.LoRa.SpreadingFactor = SX126x::RadioLoRaSpreadingFactors_t::LORA_SF12;
  radio.SetModulationParams(modulationParams);



  static const char* message = "Hello, I'm a Pinephone!";
  auto s = strlen(message);

  SX126x::PacketParams_t packetParams;
  packetParams.PacketType = SX126x::RadioPacketTypes_t::PACKET_TYPE_LORA;;
  packetParams.Params.LoRa.HeaderType = SX126x::RadioLoRaPacketLengthsMode_t::LORA_PACKET_VARIABLE_LENGTH;
  packetParams.Params.LoRa.InvertIQ = SX126x::RadioLoRaIQModes_t::LORA_IQ_INVERTED;
  packetParams.Params.LoRa.CrcMode = SX126x::RadioLoRaCrcModes_t::LORA_CRC_ON;
  packetParams.Params.LoRa.PayloadLength = s;
  packetParams.Params.LoRa.PreambleLength = 8;
  radio.SetPacketParams(packetParams);
  radio.ClearIrqStatus(0xffff);
  radio.SetRx(0xffffffff);
}
void PinedioLoraRadio::Send(const std::vector<uint8_t> data) {
  std::cout << "[PinedioLoraRadio] Send()" << std::endl;
  dataToSend = true;
  transmitBuffer = data; //copy
}

std::vector<uint8_t> PinedioLoraRadio::Receive(std::chrono::milliseconds timeout) {
  bool running = true;
  auto startTime = std::chrono::steady_clock::now();
  std::vector<uint8_t> buffer;
  while(running) {
    radio.ProcessIrqs();

    if(dataToSend) {
      std::cout << "SEND " << std::to_string(transmitBuffer.size()) << std::endl;

      SX126x::PacketParams_t packetParams;
      packetParams.PacketType = SX126x::RadioPacketTypes_t::PACKET_TYPE_LORA;;
      packetParams.Params.LoRa.HeaderType = SX126x::RadioLoRaPacketLengthsMode_t::LORA_PACKET_VARIABLE_LENGTH;
      packetParams.Params.LoRa.InvertIQ = SX126x::RadioLoRaIQModes_t::LORA_IQ_INVERTED;
      packetParams.Params.LoRa.CrcMode = SX126x::RadioLoRaCrcModes_t::LORA_CRC_ON;
      packetParams.Params.LoRa.PayloadLength = transmitBuffer.size();
      packetParams.Params.LoRa.PreambleLength = 8;
      radio.SetPacketParams(packetParams);

      radio.SendPayload(transmitBuffer.data(), transmitBuffer.size(), 0xffffffff);
      usleep(3000000);
      dataToSend = false;
    }

    if(dataReceived) {
      buffer = receivedBuffer;
      break;
    }

    if(std::chrono::steady_clock::now() - startTime > timeout)
      break;

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  dataReceived = false;
  return buffer; //copy
}

void PinedioLoraRadio::OnDataReceived() {
  uint8_t size = 0;
  uint8_t offset = 0;
  radio.GetRxBufferStatus(&size, &offset);

  receivedBuffer.clear();
  receivedBuffer.resize(size);
  radio.GetPayload(receivedBuffer.data(), &size, size);

  dataReceived = true;
}
