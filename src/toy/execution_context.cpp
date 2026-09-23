#include "../../include/toy/execution_context.hpp"

#include <fstream>
#include <vector>

void ExecutionContext::loadBinary(const std::string &Filename,
                                  uint32_t LoadAddress) {
  std::ifstream File(Filename, std::ios::binary | std::ios::ate);
  if (!File) {
    throw SimulationException(ErrorCode::INCORRECT_BINARY_FILE,
                              "[LOADER] Cannot open binary file: " + Filename);
  }

  auto EndPosition = File.tellg();
  if (EndPosition == std::ifstream::pos_type(-1)) {
    throw SimulationException(ErrorCode::INCORRECT_BINARY_FILE,
                              "[LOADER] Cannot determine binary size: " +
                                  Filename);
  }

  std::streamoff FileSize = EndPosition - std::ifstream::pos_type(0);
  if (FileSize <= 0 || FileSize % 4 != 0) {
    throw SimulationException(
        ErrorCode::INCORRECT_BINARY_FILE,
        "[LOADER] Binary must contain a nonzero multiple of 4 bytes: " +
            Filename);
  }

  if (LoadAddress % 4 != 0) {
    throw SimulationException(ErrorCode::MISALIGNED_ACCESS,
                              "[LOADER] Misaligned load address: " +
                                  std::to_string(LoadAddress));
  }

  File.seekg(0, std::ios::beg);

  std::vector<unsigned char> Buffer(FileSize, 0);
  if (!File.read(reinterpret_cast<char *>(Buffer.data()),
                 static_cast<std::streamsize>(Buffer.size()))) {
    throw SimulationException(ErrorCode::INCORRECT_BINARY_FILE,
                              "[LOADER] Cannot read complete binary: " +
                                  Filename);
  }

  if (LoadAddress > Memory.size() ||
      Buffer.size() > Memory.size() - LoadAddress) {
    throw SimulationException(
        ErrorCode::MEMORY_OUT_OF_BOUNDS,
        "[LOADER] Binary does not fit in memory at load address: " +
            std::to_string(LoadAddress));
  }

  Memory.clearInstructionProtection();

  for (std::size_t Offset = 0; Offset < Buffer.size(); Offset += 4) {
    uint32_t Word    = static_cast<uint32_t>(Buffer[Offset]) |
                       (static_cast<uint32_t>(Buffer[Offset + 1]) << 8) |
                       (static_cast<uint32_t>(Buffer[Offset + 2]) << 16) |
                       (static_cast<uint32_t>(Buffer[Offset + 3]) << 24);
    uint32_t Address = static_cast<uint32_t>(LoadAddress + Offset);
    Memory.write32(Address, Word);
  }
  
  Memory.protectInstructionRange(LoadAddress, Buffer.size());

  Cpu.PC = LoadAddress;
  State.reset();
  BlockCache.clear();
}

void ExecutionContext::step() {
  const TIBasicBlock &Block = BlockCache.getBlock(Cpu.PC, Memory, Decoder);
  executeBlock(Block);
}

void ExecutionContext::run() {
  while (State.Status == ExecutionStatus::RUNNING) {
    step();
  }
}

void ExecutionContext::executeBlock(const TIBasicBlock &Block) {
  for (const TIInstruction &Instruction : Block.instructions()) {
    execute(Instruction);
    if (State.Status != ExecutionStatus::RUNNING) {
      return;
    }
  }
}
