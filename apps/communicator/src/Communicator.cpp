#include <PineDio/LoRa/Communicator.h>
#include <iostream>
#include <future>
#include <algorithm>
using namespace PineDio::LoRa;

namespace {
std::string GetString() {
  std::string input;
  std::getline(std::cin, input);
  return input;
}
}

Communicator::Communicator(PineDio::LoRa::PinedioLoraRadio &radio) : radio{radio} {
  radio.Initialize();
  receiveTask.reset(new std::thread([this](){Receive();}));
}

Communicator::~Communicator() {
  running = false;
  receiveTask->join();
}


void Communicator::Run() {
  running = true;

  std::future<std::string> futureString = std::async(std::launch::async, GetString);


  while(running) {
    if(futureString.wait_for(std::chrono::milliseconds {0}) == std::future_status::ready) {
      auto msgStr = futureString.get();
      std::vector<uint8_t> msg;
      for(auto c : msgStr)
        msg.push_back(c);
      msg.push_back('\0');
      radio.Send(msg);
      futureString = std::async(std::launch::async, GetString);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds{100});
  }

}
void Communicator::Receive() {
  while(running){
    auto data = radio.Receive(std::chrono::milliseconds{100});
    if(data.empty())
      continue;
    std::cout << "Data received on LoRa radio : " << std::endl;
    std::cout << "\tHEX: ";
    for(auto d : data) {
      int dd = d;
      std::cout << std::hex << "0x" << dd << " ";
    }
    std::cout << std::endl << "\tSTR: ";
    for(auto d : data) {
      std::cout << d;
    }
    std::cout << std::endl;
  }
}

void Communicator::Stop() {
  running = false;
}

