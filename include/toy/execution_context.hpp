#pragma once

#include "cpu_state.hpp"
#include "decoder.hpp"
#include "execution_state.hpp"
#include "memory.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

class ExecutionContext {
public:
  explicit ExecutionContext(std::size_t MemorySize) : Memory(MemorySize) {}

  /// Load complete 32-bit words, set PC and reset execution status.
  void loadBinary(const std::string &Filename, uint32_t LoadAddress = 0);

  /// Fetch, decode and execute one instruction.
  void step();

  /// Execute instructions while the execution status is RUNNING.
  void run();

private:
  TICpuState Cpu{};
  TIMemory Memory;
  TIExecutionState State{};
  TIDecoder Decoder{};

  uint32_t fetch() const { return Memory.read32(Cpu.PC); }

  void execute(const TIInstruction &Inst);
};
