#pragma once

#include "cpu_state.hpp"
#include "execution_state.hpp"
#include "instruction.hpp"
#include "memory.hpp"
#include "syscall_emulator.hpp"

struct TIThreadState final {
  TIThreadState(TICpuState &CpuState, TIMemory &MemoryState,
                TIExecutionState &ExecutionState,
                TISyscallEmulator &SyscallState,
                const TIInstruction *FirstInstruction,
                const TIInstruction *EndInstruction)
      : Cpu(CpuState), Memory(MemoryState), State(ExecutionState),
        SyscallEmulator(SyscallState), Current(FirstInstruction),
        End(EndInstruction) {}

  TICpuState &Cpu;
  TIMemory &Memory;
  TIExecutionState &State;
  TISyscallEmulator &SyscallEmulator;
  const TIInstruction *Current = nullptr;
  const TIInstruction *End     = nullptr;
};

/// Return the threaded execution handler for an internal opcode.
TIExecuteHandler getExecuteHandler(TIOpcode OpCode);
