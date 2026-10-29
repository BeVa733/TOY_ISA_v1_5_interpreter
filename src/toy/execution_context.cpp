#include "toy/execution_context.hpp"
#include "execution_type/jit_executor.hpp"
#include "execution_type/switch_executor.hpp"
#include "execution_type/threaded_executor.hpp"
#include "toy/toy_constants.hpp"

#include <elfio/elfio.hpp>

#include <cstddef>
#include <cstring>

namespace {

void loadSegment(const ELFIO::segment *Segment, TIMemory &Memory) {
  const ELFIO::Elf64_Addr Address   = Segment->get_virtual_address();
  const ELFIO::Elf_Xword FileSize   = Segment->get_file_size();
  const ELFIO::Elf_Xword MemorySize = Segment->get_memory_size();

  if (FileSize > MemorySize) {
    throw SimulationException(
        ErrorCode::INCORRECT_BINARY_FILE,
        "[LOADER] ELF segment file size exceeds memory size");
  }

  TIMemoryRange Range = Memory.writableRange(
      static_cast<uint32_t>(Address), static_cast<std::size_t>(MemorySize));
  if (FileSize != 0) {
    std::memcpy(Range.Data, Segment->get_data(),
                static_cast<std::size_t>(FileSize));
  }
  if (MemorySize > FileSize) {
    std::memset(Range.Data + FileSize, 0,
                static_cast<std::size_t>(MemorySize - FileSize));
  }
}

} // namespace

void ExecutionContext::loadBinary(const std::string &Filename,
                                  uint32_t LoadAddress) {
  if (LoadAddress != 0) {
    throw SimulationException(ErrorCode::MISALIGNED_ACCESS,
                              "[LOADER] ELF does not support a load address: " +
                                  std::to_string(LoadAddress));
  }

  ELFIO::elfio Reader{};
  if (!Reader.load(Filename)) {
    throw SimulationException(ErrorCode::INCORRECT_BINARY_FILE,
                              "[LOADER] Cannot read ELF file: " + Filename);
  }

  if (Reader.get_type() != ELFIO::ET_EXEC ||
      Reader.get_class() != ELFIO::ELFCLASS32 ||
      Reader.get_encoding() != ELFIO::ELFDATA2LSB ||
      Reader.get_machine() != TI32::ELF_MACHINE) {
    throw SimulationException(
        ErrorCode::INCORRECT_BINARY_FILE,
        "[LOADER] Expected a 32-bit little-endian Toy ISA ELF");
  }

  bool HasExecutableSegment = false;
  for (const auto &SegmentHandle : Reader.segments) {
    const ELFIO::segment *Segment = SegmentHandle.get();
    if (Segment->get_type() != ELFIO::PT_LOAD) {
      continue;
    }

    if ((Segment->get_flags() & ELFIO::PF_X) != 0 &&
        Segment->get_memory_size() != 0) {
      if (Segment->get_memory_size() % TI32::INSTRUCTION_SIZE != 0 ||
          Segment->get_virtual_address() % TI32::INSTRUCTION_SIZE != 0) {
        throw SimulationException(
            ErrorCode::INCORRECT_BINARY_FILE,
            "[LOADER] Executable ELF segment is not instruction-aligned");
      }
      HasExecutableSegment = true;
    }

    loadSegment(Segment, Memory);
  }

  if (!HasExecutableSegment) {
    throw SimulationException(
        ErrorCode::INCORRECT_BINARY_FILE,
        "[LOADER] ELF must contain a nonempty executable load segment");
  }

  Cpu.PC = static_cast<uint32_t>(Reader.get_entry());
  State.reset();
  BlockCache.clear();
}

void ExecutionContext::step() {
  TIBasicBlock &Block = BlockCache.getBlock(Cpu.PC, Memory, Decoder);
  executeBlock(Block);
}

void ExecutionContext::run() {
  while (State.Status == ExecutionStatus::RUNNING) {
    step();
  }
}

void ExecutionContext::executeBlock(TIBasicBlock &Block) {
  try {
    switch (Mode) {
    case TIExecutionMode::SWITCH:
      executeSwitchBlock(*this, Block);
      return;
    case TIExecutionMode::JIT:
      executeJitBlock(*this, BlockCache, Block, JitThreshold_);
      return;
    case TIExecutionMode::THREADED:
      executeThreadedBlock(*this, Block);
      return;
    }
  } catch (const SimulationException &) {
    State.fault();
    throw;
  }
}
