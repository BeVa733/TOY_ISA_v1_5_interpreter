#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

class Memory {
public:
  explicit Memory(std::size_t SizeBytes);

  std::size_t size() const;

  uint8_t read8(uint32_t Address) const;
  void write8(uint32_t Address, uint8_t Value);

  uint32_t read32(uint32_t Address) const;
  void write32(uint32_t Address, uint32_t Value);

private:
  std::vector<uint8_t> Bytes_{};

  void checkBounds(uint32_t Address, std::size_t ByteCount) const;
  void checkAlignment(uint32_t Address) const;
};
