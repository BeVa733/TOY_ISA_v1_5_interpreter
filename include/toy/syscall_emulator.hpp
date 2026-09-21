#pragma once

#include "cpu_state.hpp"
#include "execution_state.hpp"
#include "memory.hpp"

#include <cstdint>
#include <unordered_map>

class TISyscallEmulator {
public:
  void execute(TICpuState &Cpu, TIMemory &Memory,
               TIExecutionState &State) const;

private:
  using Handler = void (*)(TICpuState &, TIMemory &, TIExecutionState &);

  static void handleRead(TICpuState &Cpu, TIMemory &Memory,
                         TIExecutionState &State);
  static void handleWrite(TICpuState &Cpu, TIMemory &Memory,
                          TIExecutionState &State);
  static void handleExit(TICpuState &Cpu, TIMemory &Memory,
                         TIExecutionState &State);

  const std::unordered_map<uint32_t, Handler> Handlers{
      {0,  handleRead },
      {1,  handleWrite},
      {60, handleExit }
  };
};
