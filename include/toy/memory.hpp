#pragma once

#include <cstddef>
#include <cstdint>
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
  explicit TIMemory(std::size_t SizeBytes) : Bytes(SizeBytes, 0) {}

  std::size_t size() const { return Bytes.size(); }

  /// Return true if instructions are protected
  bool hasInstructionRange() const { return InstructionProtectionEnabled_; }

  /// Return true if instruction belongs to instruction range
  bool containsInstruction(uint32_t Address) const {
    uint64_t Begin = Address;
    uint64_t End   = static_cast<uint64_t>(Address) + TI32::INSTRUCTION_SIZE;
    return InstructionProtectionEnabled_ && Begin >= InstructionBegin_ &&
           End <= InstructionEnd_;
  }

  /// Mark a byte range as executable code and prohibit writes to it.
  void protectInstructionRange(uint32_t Address, std::size_t ByteCount) {
    checkBounds(Address, ByteCount);
    InstructionBegin_             = Address;
    InstructionEnd_               = static_cast<uint64_t>(Address) + ByteCount;
    InstructionProtectionEnabled_ = true;
  }

  /// Remove protection from the previously marked instruction range.
  void clearInstructionProtection() {
    InstructionBegin_             = 0;
    InstructionEnd_               = 0;
    InstructionProtectionEnabled_ = false;
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
    checkWritable(Address, ByteCount);
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
  uint64_t InstructionBegin_         = 0;
  uint64_t InstructionEnd_           = 0;
  bool InstructionProtectionEnabled_ = false;

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

  void checkWritable(uint32_t Address, std::size_t ByteCount) const {
    if (!InstructionProtectionEnabled_ || ByteCount == 0) {
      return;
    }

    uint64_t WriteBegin = Address;
    uint64_t WriteEnd   = static_cast<uint64_t>(Address) + ByteCount;
    if (WriteBegin < InstructionEnd_ && WriteEnd > InstructionBegin_) {
      throw SimulationException(
          ErrorCode::WRITE_TO_CODE,
          "[MEMORY] Error: write to instruction memory at address " +
              std::to_string(Address));
    }
  }
};
