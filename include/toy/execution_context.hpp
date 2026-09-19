#pragma once

#include "cpu_state.hpp"
#include "execution_state.hpp"
#include "instruction.hpp"
#include "memory.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

class ExecutionContext {
public:
  explicit ExecutionContext(std::size_t MemorySize) : Mem(MemorySize) {}

  /// Load complete 32-bit words, then set PC and reset execution status.
  /// Empty files are rejected; registers and memory outside the image are kept.
  void loadBinary(const std::string &Filename, uint32_t LoadAddress = 0);

  /// Fetch, decode and execute one instruction.
  void step();

  /// Execute instructions while the execution status is RUNNING.
  void run();

private:
  CpuState Cpu{};
  /// Initialize with Mem(MemorySize) in the context constructor.
  Memory Mem;
  ExecutionState State{};

  uint32_t fetch() const;
  static Instruction decode(uint32_t Word);
  void execute(const Instruction &Inst);
};
