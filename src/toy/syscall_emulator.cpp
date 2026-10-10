#include "../../include/toy/syscall_emulator.hpp"

#include <cerrno>
#include <cstdint>
#include <iostream>
#include <string>
#include <unistd.h>

namespace {

uint32_t syscallError() { return uint32_t{0} - static_cast<uint32_t>(errno); }

} // namespace

void TISyscallEmulator::execute(TICpuState &Cpu, TIMemory &Memory,
                                TIExecutionState &State) const {
  uint32_t SyscallNumber = Cpu.Registers[8];
  auto It                = Handlers.find(SyscallNumber);

  if (It == Handlers.end()) {
    throw SimulationException(ErrorCode::UNSUPPORTED_SYSCALL,
                              "[SYSCALL] Unsupported syscall: " +
                                  std::to_string(SyscallNumber));
  }

  It->second(Cpu, Memory, State);
}

void TISyscallEmulator::handleRead(TICpuState &Cpu, TIMemory &Memory,
                                   TIExecutionState &) {
  uint32_t GuestFileDescriptor = Cpu.Registers[0];
  uint32_t Address             = Cpu.Registers[1];
  uint32_t ByteCount           = Cpu.Registers[2];

  if (GuestFileDescriptor != STDIN_FILENO) {
    Cpu.Registers[0] = uint32_t{0} - static_cast<uint32_t>(EBADF);
    return;
  }

  TIMemoryRange Buffer = Memory.writableRange(Address, ByteCount);
  ssize_t Result       = read(STDIN_FILENO, Buffer.Data, Buffer.Size);

  if (Result < 0) {
    Cpu.Registers[0] = syscallError();
    return;
  }

  Cpu.Registers[0] = static_cast<uint32_t>(Result);
}

void TISyscallEmulator::handleReadNumber(TICpuState &Cpu, TIMemory &,
                                         TIExecutionState &) {
  int32_t Value = 0;
  if (std::cin >> Value) {
    Cpu.Registers[0] = static_cast<uint32_t>(Value);
    return;
  }

  errno = std::cin.eof() ? ENODATA : EINVAL;
  Cpu.Registers[0] = syscallError();
}

void TISyscallEmulator::handleWrite(TICpuState &Cpu, TIMemory &Memory,
                                    TIExecutionState &) {
  uint32_t GuestFileDescriptor = Cpu.Registers[0];
  uint32_t Address             = Cpu.Registers[1];
  uint32_t ByteCount           = Cpu.Registers[2];

  if (GuestFileDescriptor != STDOUT_FILENO &&
      GuestFileDescriptor != STDERR_FILENO) {
    Cpu.Registers[0] = uint32_t{0} - static_cast<uint32_t>(EBADF);
    return;
  }

  TIConstMemoryRange Buffer = Memory.readableRange(Address, ByteCount);
  ssize_t Result =
      write(static_cast<int>(GuestFileDescriptor), Buffer.Data, Buffer.Size);

  Cpu.Registers[0] =
      Result < 0 ? syscallError() : static_cast<uint32_t>(Result);
}

void TISyscallEmulator::handleExit(TICpuState &Cpu, TIMemory &,
                                   TIExecutionState &State) {
  State.halt(Cpu.Registers[0]);
}
