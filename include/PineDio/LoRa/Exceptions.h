#pragma once

namespace PineDio::LoRa {
class InitializationException : public std::runtime_error {
public:
  explicit InitializationException(const std::string& message) : runtime_error(message) {}
};

}
