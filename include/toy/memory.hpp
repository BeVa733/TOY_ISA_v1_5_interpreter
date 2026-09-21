#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "execution_state.hpp"

class TIMemory {
public:
  explicit TIMemory(std::size_t SizeBytes) : Bytes(SizeBytes, 0) {}

  std::size_t size() const { return Bytes.size(); }

  uint32_t read32(uint32_t Address) const {

    checkBounds(Address, 4);
    checkAlignment(Address);

    return (static_cast<uint32_t>(Bytes[Address])) |
           (static_cast<uint32_t>(Bytes[Address + 1]) << 8) |
           (static_cast<uint32_t>(Bytes[Address + 2]) << 16) |
           (static_cast<uint32_t>(Bytes[Address + 3]) << 24);
  }

  void write32(uint32_t Address, uint32_t Value) {

    checkBounds(Address, 4);
    checkAlignment(Address);

    Bytes[Address]     = static_cast<uint8_t>(Value & 0xFF);
    Bytes[Address + 1] = static_cast<uint8_t>((Value >> 8) & 0xFF);
    Bytes[Address + 2] = static_cast<uint8_t>((Value >> 16) & 0xFF);
    Bytes[Address + 3] = static_cast<uint8_t>((Value >> 24) & 0xFF);
  }

private:
  std::vector<uint8_t> Bytes{};

  void checkBounds(uint32_t Address, std::size_t ByteCount) const {
    if ((Address + ByteCount) > Bytes.size()) {
      throw SimulationException(
          ErrorCode::MEMORY_OUT_OF_BOUNDS,
          "[MEMORY] Error: nemory out of bounds at address " +
              std::to_string(Address));
    }
  }

  void checkAlignment(uint32_t Address) const {
    if (Address % 4 != 0) {
      throw SimulationException(
          ErrorCode::MISALIGNED_ACCESS,
          "[MEMORY] Error: misaligned access at " +
              std::to_string(Address));
    }
  }
};
