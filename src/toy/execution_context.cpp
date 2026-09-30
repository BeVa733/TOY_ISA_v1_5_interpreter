#include "../../include/toy/execution_context.hpp"
#include "../../include/toy/toy_constants.hpp"

#include <cstddef>
#include <fstream>

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
  if (FileSize <= 0 || FileSize % TI32::WORD_SIZE != 0) {
    throw SimulationException(
        ErrorCode::INCORRECT_BINARY_FILE,
        "[LOADER] Binary must contain a nonzero multiple of 4 bytes: " +
            Filename);
  }

  if (LoadAddress % TI32::WORD_SIZE != 0) {
    throw SimulationException(ErrorCode::MISALIGNED_ACCESS,
                              "[LOADER] Misaligned load address: " +
                                  std::to_string(LoadAddress));
  }

  File.seekg(0, std::ios::beg);

  std::size_t ByteCount = FileSize;
  if (LoadAddress > Memory.size() || ByteCount > Memory.size() - LoadAddress) {
    throw SimulationException(
        ErrorCode::MEMORY_OUT_OF_BOUNDS,
        "[LOADER] Binary does not fit in memory at load address: " +
            std::to_string(LoadAddress));
  }

  Memory.clearInstructionProtection();
  TIMemoryRange Program = Memory.writableRange(LoadAddress, ByteCount);

  if (!File.read(reinterpret_cast<char *>(Program.Data),
                 static_cast<std::streamsize>(Program.Size))) {
    throw SimulationException(ErrorCode::INCORRECT_BINARY_FILE,
                              "[LOADER] Cannot read complete binary: " +
                                  Filename);
  }

  Memory.protectInstructionRange(LoadAddress, ByteCount);

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
  const auto &Instructions = Block.instructions();
  TIThreadState Thread(Cpu, Memory, State, SyscallEmulator, Instructions.data(),
                       Instructions.data() + Instructions.size());

  try {
    Thread.Current->Execute(Thread);
  } catch (const SimulationException &) {
    State.fault();
    throw;
  }
}
