#include "../../include/toy/executor.hpp"
#include "../../include/toy/toy_constants.hpp"

#if !defined(__clang__)
#error "Threaded execution with [[clang::musttail]] requires Clang"
#endif

namespace {

#define DISPATCH_NEXT(Thread)                                                  \
  Thread.Cpu.PC += TI32::INSTRUCTION_SIZE;                                     \
  ++(Thread).Current;                                                          \
  if ((Thread).Current == (Thread).End) {                                      \
    return;                                                                    \
  }                                                                            \
  [[clang::musttail]] return (Thread).Current->Execute(Thread)

uint32_t countLeadingZeros(uint32_t Value) {
  uint32_t Count = 0;
  for (uint32_t Mask = 0x80000000; Mask != 0 && (Value & Mask) == 0;
       Mask >>= 1) {
    ++Count;
  }
  return Count;
}

uint32_t saturateSigned(uint32_t Value, uint32_t Width) {
  if (Width == 0 || Width >= TI32::WORD_BIT_COUNT) {
    throw SimulationException(ErrorCode::INVALID_INSTRUCTION,
                              "[EXECUTE] SSAT width must be in range 1..31");
  }

  uint32_t Maximum = makeMask(Width - 1); // -1 because without sign bit
  uint32_t Minimum = ~Maximum;
  uint32_t SignBit = uint32_t{1} << (TI32::WORD_BIT_COUNT - 1);

  if ((Value & SignBit) == 0) {
    return Value > Maximum ? Maximum : Value;
  }

  return Value < Minimum ? Minimum : Value;
}

uint32_t rotateRight(uint32_t Value, uint32_t Shift) {
  if (Shift == 0) {
    return Value;
  }
  return (Value >> Shift) | (Value << (TI32::WORD_BIT_COUNT - Shift));
}

uint32_t compressBits(uint32_t Value, uint32_t Mask) {
  uint32_t Result    = 0;
  uint32_t OutputBit = 0;
  for (uint32_t InputBit = 0; InputBit < TI32::WORD_BIT_COUNT; ++InputBit) {
    if ((Mask & (uint32_t{1} << InputBit)) != 0) {
      Result |= ((Value >> InputBit) & 0x01) << OutputBit;
      ++OutputBit;
    }
  }
  return Result;
}

void executeLi(TIThreadState &Thread) {
  const auto &Operands                    = Thread.Current->Operands;
  Thread.Cpu.Registers[Operands[0].Value] = Operands[1].Value;
  DISPATCH_NEXT(Thread);
}

void executeAdd(TIThreadState &Thread) {
  const auto &Operands = Thread.Current->Operands;
  auto &Registers      = Thread.Cpu.Registers;
  Registers[Operands[0].Value] =
      Registers[Operands[1].Value] + Registers[Operands[2].Value];
  DISPATCH_NEXT(Thread);
}

void executeAddi(TIThreadState &Thread) {
  const auto &Operands = Thread.Current->Operands;
  auto &Registers      = Thread.Cpu.Registers;
  Registers[Operands[0].Value] =
      Registers[Operands[1].Value] + Operands[2].Value;
  DISPATCH_NEXT(Thread);
}

void executeOr(TIThreadState &Thread) {
  const auto &Operands = Thread.Current->Operands;
  auto &Registers      = Thread.Cpu.Registers;
  Registers[Operands[0].Value] =
      Registers[Operands[1].Value] | Registers[Operands[2].Value];
  DISPATCH_NEXT(Thread);
}

void executeLdReg(TIThreadState &Thread) {
  const auto &Operands = Thread.Current->Operands;
  auto &Registers      = Thread.Cpu.Registers;
  uint32_t Address =
      Registers[Operands[1].Value] + Registers[Operands[2].Value];
  Registers[Operands[0].Value] = Thread.Memory.read32(Address);
  DISPATCH_NEXT(Thread);
}

void executeLdImm(TIThreadState &Thread) {
  const auto &Operands = Thread.Current->Operands;
  auto &Registers      = Thread.Cpu.Registers;
  uint32_t Address     = Registers[Operands[1].Value] + Operands[2].Value;
  Registers[Operands[0].Value] = Thread.Memory.read32(Address);
  DISPATCH_NEXT(Thread);
}

void executeSt(TIThreadState &Thread) {
  const auto &Operands = Thread.Current->Operands;
  auto &Registers      = Thread.Cpu.Registers;
  uint32_t Address     = Registers[Operands[1].Value] + Operands[2].Value;
  Thread.Memory.write32(Address, Registers[Operands[0].Value]);
  DISPATCH_NEXT(Thread);
}

void executeStp(TIThreadState &Thread) {
  const auto &Operands = Thread.Current->Operands;
  auto &Registers      = Thread.Cpu.Registers;
  uint32_t Address     = Registers[Operands[2].Value] + Operands[3].Value;

  Thread.Memory.write32(Address, Registers[Operands[0].Value]);
  Thread.Memory.write32(Address + TI32::WORD_SIZE,
                        Registers[Operands[1].Value]);

  DISPATCH_NEXT(Thread);
}

void executeBeq(TIThreadState &Thread) {
  const auto &Operands = Thread.Current->Operands;
  auto &Registers      = Thread.Cpu.Registers;
  if (Registers[Operands[0].Value] == Registers[Operands[1].Value]) {
    Thread.Cpu.PC += Operands[2].Value * TI32::INSTRUCTION_SIZE;
  } else {
    Thread.Cpu.PC += TI32::INSTRUCTION_SIZE;
  }
}

void executeJ(TIThreadState &Thread) {
  const auto &Operands = Thread.Current->Operands;
  Thread.Cpu.PC        = (Thread.Cpu.PC & 0xF0000000) |
                         (Operands[0].Value * TI32::INSTRUCTION_SIZE);
}

void executeClz(TIThreadState &Thread) {
  const auto &Operands = Thread.Current->Operands;
  auto &Registers      = Thread.Cpu.Registers;
  Registers[Operands[0].Value] =
      countLeadingZeros(Registers[Operands[1].Value]);
  DISPATCH_NEXT(Thread);
}

void executeSsat(TIThreadState &Thread) {
  const auto &Operands = Thread.Current->Operands;
  auto &Registers      = Thread.Cpu.Registers;
  Registers[Operands[0].Value] =
      saturateSigned(Registers[Operands[1].Value], Operands[2].Value);
  DISPATCH_NEXT(Thread);
}

void executeRori(TIThreadState &Thread) {
  const auto &Operands = Thread.Current->Operands;
  auto &Registers      = Thread.Cpu.Registers;
  Registers[Operands[0].Value] =
      rotateRight(Registers[Operands[1].Value], Operands[2].Value);
  DISPATCH_NEXT(Thread);
}

void executeBext(TIThreadState &Thread) {
  const auto &Operands = Thread.Current->Operands;
  auto &Registers      = Thread.Cpu.Registers;
  Registers[Operands[0].Value] =
      compressBits(Registers[Operands[1].Value], Registers[Operands[2].Value]);
  DISPATCH_NEXT(Thread);
}

void executeSyscall(TIThreadState &Thread) {
  Thread.SyscallEmulator.execute(Thread.Cpu, Thread.Memory, Thread.State);
  if (Thread.State.Status == ExecutionStatus::RUNNING) {
    Thread.Cpu.PC += TI32::INSTRUCTION_SIZE;
  }
}

void executeInvalid(TIThreadState &) {
  throw SimulationException(ErrorCode::INVALID_INSTRUCTION,
                            "[EXECUTE] Invalid instruction");
}

#undef DISPATCH_NEXT

} // namespace

TIExecuteHandler getExecuteHandler(TIOpcode OpCode) {
  switch (OpCode) {
  case TIOpcode::LI:
    return executeLi;
  case TIOpcode::ADD:
    return executeAdd;
  case TIOpcode::ADDI:
    return executeAddi;
  case TIOpcode::OR:
    return executeOr;
  case TIOpcode::LDreg:
    return executeLdReg;
  case TIOpcode::ST:
    return executeSt;
  case TIOpcode::STP:
    return executeStp;
  case TIOpcode::BEQ:
    return executeBeq;
  case TIOpcode::J:
    return executeJ;
  case TIOpcode::CLZ:
    return executeClz;
  case TIOpcode::SSAT:
    return executeSsat;
  case TIOpcode::RORI:
    return executeRori;
  case TIOpcode::BEXT:
    return executeBext;
  case TIOpcode::SYSCALL:
    return executeSyscall;
  case TIOpcode::LDimm:
    return executeLdImm;
  case TIOpcode::INVALID:
  default:
    return executeInvalid;
  }
}
