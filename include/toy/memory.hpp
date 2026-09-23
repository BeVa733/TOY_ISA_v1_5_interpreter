#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "execution_state.hpp"

class TIMemory {
public:
  explicit TIMemory(std::size_t SizeBytes) : Bytes(SizeBytes, 0) {}

  std::size_t size() const { return Bytes.size(); }

  /// Return true if instructions are protected
  bool hasInstructionRange() const { return InstructionProtectionEnabled_; }

  /// Return true if instruction belongs to instruction range
  bool containsInstruction(uint32_t Address) const {
    uint64_t Begin = Address;
    uint64_t End   = static_cast<uint64_t>(Address) + 4;
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

  /// Copy an arbitrary byte range from guest memory.
  std::vector<uint8_t> readBytes(uint32_t Address,
                                 std::size_t ByteCount) const {
    checkBounds(Address, ByteCount);

    auto Begin = Bytes.begin() + Address;
    return {Begin, Begin + ByteCount};
  }

  /// Copy bytes into guest memory. Byte accesses do not require alignment.
  void writeBytes(uint32_t Address, const std::vector<uint8_t> &Data) {
    checkBounds(Address, Data.size());
    checkWritable(Address, Data.size());

    for (std::size_t Index = 0; Index < Data.size(); ++Index) {
      Bytes[Address + Index] = Data[Index];
    }
  }

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
    checkWritable(Address, 4);

    Bytes[Address]     = static_cast<uint8_t>(Value & 0xFF);
    Bytes[Address + 1] = static_cast<uint8_t>((Value >> 8) & 0xFF);
    Bytes[Address + 2] = static_cast<uint8_t>((Value >> 16) & 0xFF);
    Bytes[Address + 3] = static_cast<uint8_t>((Value >> 24) & 0xFF);
  }

  /// Store two words
  void writePair32(uint32_t Address, uint32_t First, uint32_t Second) {
    checkBounds(Address, 8);
    checkAlignment(Address);
    checkWritable(Address, 8);
    write32(Address, First);
    write32(Address + 4, Second);
  }

private:
  std::vector<uint8_t> Bytes{};
  uint64_t InstructionBegin_         = 0;
  uint64_t InstructionEnd_           = 0;
  bool InstructionProtectionEnabled_ = false;

  void checkBounds(uint32_t Address, std::size_t ByteCount) const {
    constexpr uint64_t ADDRESS_SPACE_SIZE = uint64_t{1} << 32;
    if (Address > Bytes.size() || ByteCount > Bytes.size() - Address ||
        ByteCount > ADDRESS_SPACE_SIZE - Address) {
      throw SimulationException(
          ErrorCode::MEMORY_OUT_OF_BOUNDS,
          "[MEMORY] Error: nemory out of bounds at address " +
              std::to_string(Address));
    }
  }

  void checkAlignment(uint32_t Address) const {
    if (Address % 4 != 0) {
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
