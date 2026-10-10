#include "../../include/toy/execution_context.hpp"
#include "../../include/toy/toy_constants.hpp"

#include <elfio/elfio.hpp>

#include <cstddef>
#include <cstring>

namespace {

void loadSection(const ELFIO::section *Section, TIMemory &Memory,
                 const char *Name) {
  if (Section == nullptr) {
    throw SimulationException(ErrorCode::INCORRECT_BINARY_FILE,
                              std::string("[LOADER] ELF section is missing: ") +
                                  Name);
  }

  const ELFIO::Elf64_Addr Address = Section->get_address();
  const ELFIO::Elf_Xword Size     = Section->get_size();

  TIMemoryRange Range = Memory.writableRange(static_cast<uint32_t>(Address),
                                             static_cast<std::size_t>(Size));
  if (Size != 0) {
    std::memcpy(Range.Data, Section->get_data(),
                static_cast<std::size_t>(Size));
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

  const ELFIO::section *Text = Reader.sections[".text"];
  const ELFIO::section *Data = Reader.sections[".data"];
  if (Text == nullptr || Text->get_size() == 0 ||
      Text->get_size() % TI32::INSTRUCTION_SIZE != 0 ||
      Text->get_address() % TI32::INSTRUCTION_SIZE != 0) {
    throw SimulationException(
        ErrorCode::INCORRECT_BINARY_FILE,
        "[LOADER] ELF must contain an aligned nonempty .text section");
  }

  loadSection(Text, Memory, ".text");
  if (Data != nullptr) {
    loadSection(Data, Memory, ".data");
  }

  Cpu.PC = static_cast<uint32_t>(Reader.get_entry());
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

  if (Mode == TIExecutionMode::SWITCH) {
    try {
      for (const TIInstruction &Instruction : Instructions) {
        TIThreadState Thread(Cpu, Memory, State, SyscallEmulator, &Instruction,
                             &Instruction + 1);
        executeInstructionSwitch(Thread);
      }
    } catch (const SimulationException &) {
      State.fault();
      throw;
    }
    return;
  }

  TIThreadState Thread(Cpu, Memory, State, SyscallEmulator, Instructions.data(),
                       Instructions.data() + Instructions.size());

  try {
    Thread.Current->Execute(Thread);
  } catch (const SimulationException &) {
    State.fault();
    throw;
  }
}
