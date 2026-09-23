#pragma once

#include "basic_block_cache.hpp"
#include "cpu_state.hpp"
#include "decoder.hpp"
#include "execution_state.hpp"
#include "memory.hpp"
#include "syscall_emulator.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

class ExecutionContext {
public:
  explicit ExecutionContext(std::size_t MemorySize) : Memory(MemorySize) {}

  /// Load complete 32-bit words, set PC and reset execution status
  void loadBinary(const std::string &Filename, uint32_t LoadAddress = 0);

  /// Find or decode and execute one basic block
  void step();

  /// Execute instructions while the execution status is RUNNING
  void run();

  /// Const metodes for check state in tests
  const TICpuState &cpu() const { return Cpu; }
  const TIMemory &memory() const { return Memory; }
  const TIExecutionState &state() const { return State; }

private:
  TICpuState Cpu{};
  TIMemory Memory;
  TIExecutionState State{};
  TIDecoder Decoder{};
  TIBasicBlockCache BlockCache{};
  TISyscallEmulator SyscallEmulator{};

  void executeBlock(const TIBasicBlock &Block);
  void execute(const TIInstruction &Inst);
};
