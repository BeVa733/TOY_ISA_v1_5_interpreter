// Memory test for checking correctness little-endian words read/write,
// alignment, out of bounds memory access and code protection

#include "../include/toy/memory.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void require(bool Condition, const std::string &Message) {
  if (!Condition) {
    throw std::runtime_error(Message);
  }
}

template <typename TAction>
void requireError(ErrorCode Expected, TAction Action, const char *Description) {
  try {
    Action();
  } catch (const SimulationException &Error) {
    require(Error.code() == Expected,
            std::string(Description) + ": incorrect error code");
    return;
  }

  throw std::runtime_error(std::string(Description) +
                           ": expected memory error was not thrown");
}

void testWrite32() {
  TIMemory Memory(16);
  Memory.write32(4, 0x12345678);

  TIConstMemoryRange Range = Memory.readableRange(4, TI32::WORD_SIZE);
  require(Range.Data[0] == 0x78, "write32 wrote incorrect byte 0");
  require(Range.Data[1] == 0x56, "write32 wrote incorrect byte 1");
  require(Range.Data[2] == 0x34, "write32 wrote incorrect byte 2");
  require(Range.Data[3] == 0x12, "write32 wrote incorrect byte 3");
}

void testRead32() {
  TIMemory Memory(16);
  TIMemoryRange Range = Memory.writableRange(4, TI32::WORD_SIZE);
  Range.Data[0]       = 0x78;
  Range.Data[1]       = 0x56;
  Range.Data[2]       = 0x34;
  Range.Data[3]       = 0x12;

  require(Memory.read32(4) == 0x12345678,
          "read32 decoded little endian incorrectly");
}

void testWordRoundTrip() {
  TIMemory Memory(16);
  Memory.write32(8, 0x89ABCDEF);
  require(Memory.read32(8) == 0x89ABCDEF, "Word read-write test failed");
}

void testByteRanges() {
  TIMemory Memory(16);
  TIMemoryRange Writable = Memory.writableRange(3, 3);
  Writable.Data[0]       = 10;
  Writable.Data[1]       = 20;
  Writable.Data[2]       = 30;

  TIConstMemoryRange Readable = Memory.readableRange(3, 3);
  require(Readable.Data[0] == 10, "Incorrect byte range value 0");
  require(Readable.Data[1] == 20, "Incorrect byte range value 1");
  require(Readable.Data[2] == 30, "Incorrect byte range value 2");
}

void testBoundsAndAlignment() {
  TIMemory Memory(16);

  requireError(
      ErrorCode::MEMORY_OUT_OF_BOUNDS,
      [&Memory] { Memory.readableRange(15, 2); }, "Out-of-bounds read");
  requireError(
      ErrorCode::MEMORY_OUT_OF_BOUNDS,
      [&Memory] { Memory.writableRange(16, 1); }, "Out-of-bounds write");
  requireError(
      ErrorCode::MISALIGNED_ACCESS, [&Memory] { Memory.read32(2); },
      "Misaligned read32");
  requireError(
      ErrorCode::MISALIGNED_ACCESS, [&Memory] { Memory.write32(6, 1); },
      "Misaligned write32");
}

void testProtection() {
  TIMemory Memory(64);
  Memory.protectInstructionRange(16, 16);

  Memory.read32(20);
  requireError(
      ErrorCode::WRITE_TO_CODE, [&Memory] { Memory.write32(20, 1); },
      "Write inside protected code");
  requireError(
      ErrorCode::WRITE_TO_CODE, [&Memory] { Memory.writableRange(14, 4); },
      "Write overlaps code beginning");
  requireError(
      ErrorCode::WRITE_TO_CODE, [&Memory] { Memory.writableRange(30, 4); },
      "Write overlaps code end");

  Memory.writableRange(12, 4);
  Memory.writableRange(32, 4);

  Memory.clearInstructionProtection();
  Memory.write32(20, 0x12345678);
  require(Memory.read32(20) == 0x12345678,
          "Memory remained protected after clearing protection");
}

} // namespace

int main() {
  try {
    testWrite32();
    testRead32();
    testWordRoundTrip();
    testByteRanges();
    testBoundsAndAlignment();
    testProtection();
  } catch (const std::runtime_error &Error) {
    std::cerr << Error.what() << '\n';
    return 1;
  }

  std::cout << "All memory tests passed\n";
  return 0;
}
