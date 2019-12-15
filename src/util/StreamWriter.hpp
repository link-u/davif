//
// Created by psi on 2019/11/25.
//

#pragma once


#include <cstdint>
#include <vector>

namespace util {

class StreamWriter {
private:
  std::vector<uint8_t> buff_;
public:
  StreamWriter() = default;

  StreamWriter(StreamWriter &&) = default;

  StreamWriter(StreamWriter const &) = default;

  StreamWriter &operator=(StreamWriter &&) = default;

  StreamWriter &operator=(StreamWriter const &) = default;

public:
  [[nodiscard]] std::vector<uint8_t> const& buffer() const { return this->buff_; };
  void putU8(uint8_t data);
  void putU16L(uint16_t data);
  void putU32L(uint32_t data);
  void append(std::vector<uint8_t> const& data);
  void append(uint8_t const* data, size_t length);
};

}