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

enum class TIExecutionMode : uint8_t {
  THREADED = 0,
  SWITCH   = 1,
  JIT      = 2
};

class ExecutionContext {
public:
  explicit ExecutionContext(
      std::size_t MemorySize,
      TIExecutionMode ExecutionMode = TIExecutionMode::THREADED,
      uint64_t JitThreshold         = 0)
      : Memory(MemorySize), Mode(ExecutionMode), JitThreshold_(JitThreshold) {

    // this for call BB functions for this context
    Memory.setWriteObserver([this](uint32_t Address, std::size_t ByteCount) {
      BlockCache.invalidateRange(Address, ByteCount);
    });
  }

  /// Load ELF PT_LOAD segments, set PC and reset execution status
  void loadBinary(const std::string &Filename, uint32_t LoadAddress = 0);

  /// Find or decode and execute one basic block
  void step();

  /// Execute instructions while the execution status is RUNNING
  void run();

  /// State accessors
  TICpuState &cpu() { return Cpu; }
  const TICpuState &cpu() const { return Cpu; }

  TIMemory &memory() { return Memory; }
  const TIMemory &memory() const { return Memory; }

  TIExecutionState &state() { return State; }
  const TIExecutionState &state() const { return State; }

  TISyscallEmulator &syscallEmulator() { return SyscallEmulator; }

private:
  TICpuState Cpu{};
  TIMemory Memory;
  TIExecutionState State{};
  TIDecoder Decoder{};
  TIBasicBlockCache BlockCache{};
  TISyscallEmulator SyscallEmulator{};
  TIExecutionMode Mode   = TIExecutionMode::THREADED;
  uint64_t JitThreshold_ = 0;

  void executeBlock(TIBasicBlock &Block);
};
