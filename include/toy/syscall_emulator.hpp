#pragma once

#include "cpu_state.hpp"
#include "execution_state.hpp"
#include "memory.hpp"

#include <cstdint>
#include <unordered_map>

class TISyscallEmulator {
public:
  inline static constexpr uint32_t READ_NUMBER_SYSCALL = 337;

  void execute(TICpuState &Cpu, TIMemory &Memory,
               TIExecutionState &State) const;

private:
  using Handler = void (*)(TICpuState &, TIMemory &, TIExecutionState &);

  static void handleRead(TICpuState &Cpu, TIMemory &Memory,
                         TIExecutionState &State);
  static void handleReadNumber(TICpuState &Cpu, TIMemory &Memory,
                               TIExecutionState &State);
  static void handleWrite(TICpuState &Cpu, TIMemory &Memory,
                          TIExecutionState &State);
  static void handleExit(TICpuState &Cpu, TIMemory &Memory,
                         TIExecutionState &State);

  inline static const std::unordered_map<uint32_t, Handler> Handlers{
      {0,                   handleRead      },
      {1,                   handleWrite     },
      {60,                  handleExit      },
      {READ_NUMBER_SYSCALL, handleReadNumber}
  };
};
