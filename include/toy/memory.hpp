#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

#include "bitutils.hpp"
#include "execution_state.hpp"
#include "toy_constants.hpp"

/// Non-owning read-only byte range in guest memory.
struct TIConstMemoryRange {
  const uint8_t *Data = nullptr;
  std::size_t Size    = 0;
};

/// Non-owning writable byte range in guest memory.
struct TIMemoryRange {
  uint8_t *Data    = nullptr;
  std::size_t Size = 0;
};

class TIMemory {
public:
  /// Callback lambda for inform BB Cache about writing into memory
  using WriteObserver = std::function<void(uint32_t, std::size_t)>;

  explicit TIMemory(std::size_t SizeBytes) : Bytes(SizeBytes, 0) {}

  std::size_t size() const { return Bytes.size(); }

  void setWriteObserver(WriteObserver Observer) {
    this->Observer = std::move(Observer);
  }

  /// Return a checked read-only range valid while this memory is not moved.
  TIConstMemoryRange readableRange(uint32_t Address,
                                   std::size_t ByteCount) const {
    checkBounds(Address, ByteCount);
    const uint8_t *Data = ByteCount == 0 ? nullptr : Bytes.data() + Address;
    return {Data, ByteCount};
  }

  /// Return a checked writable range valid while this memory is not moved.
  TIMemoryRange writableRange(uint32_t Address, std::size_t ByteCount) {
    checkBounds(Address, ByteCount);
    if (Observer && ByteCount != 0) {
      Observer(Address, ByteCount);
    }
    uint8_t *Data = ByteCount == 0 ? nullptr : Bytes.data() + Address;
    return {Data, ByteCount};
  }

  uint32_t read32(uint32_t Address) const {
    TIConstMemoryRange Range = readableRange(Address, TI32::WORD_SIZE);
    checkAlignment(Address);
    return decodeLittleEndianWord(Range.Data);
  }

  void write32(uint32_t Address, uint32_t Value) {
    TIMemoryRange Range = writableRange(Address, TI32::WORD_SIZE);
    checkAlignment(Address);
    encodeLittleEndianWord(Range.Data, Value);
  }

private:
  std::vector<uint8_t> Bytes{};
  WriteObserver Observer{};

  void checkBounds(uint32_t Address, std::size_t ByteCount) const {
    constexpr uint64_t ADDRESS_SPACE_SIZE = uint64_t{1} << TI32::WORD_BIT_COUNT;
    if (Address > Bytes.size() || ByteCount > Bytes.size() - Address ||
        ByteCount > ADDRESS_SPACE_SIZE - Address) {
      throw SimulationException(
          ErrorCode::MEMORY_OUT_OF_BOUNDS,
          "[MEMORY] Error: nemory out of bounds at address " +
              std::to_string(Address));
    }
  }

  void checkAlignment(uint32_t Address) const {
    if (Address % TI32::WORD_SIZE != 0) {
      throw SimulationException(ErrorCode::MISALIGNED_ACCESS,
                                "[MEMORY] Error: misaligned access at " +
                                    std::to_string(Address));
    }
  }
};
