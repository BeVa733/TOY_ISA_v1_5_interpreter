#pragma once

#include "execution_context.hpp"
#include "instruction.hpp"
#include "toy_constants.hpp"

namespace ExecutorDetail {

inline uint32_t countLeadingZeros(uint32_t Value) {
  uint32_t Count = 0;
  for (uint32_t Mask = 0x80000000; Mask != 0 && (Value & Mask) == 0;
       Mask >>= 1) {
    ++Count;
  }
  return Count;
}

inline uint32_t saturateSigned(uint32_t Value, uint32_t Width) {
  if (Width == 0 || Width >= TI32::WORD_BIT_COUNT) {
    throw SimulationException(ErrorCode::INVALID_INSTRUCTION,
                              "[EXECUTE] SSAT width must be in range 1..31");
  }

  uint32_t Maximum = makeMask(Width - 1);
  uint32_t Minimum = ~Maximum;
  uint32_t SignBit = uint32_t{1} << (TI32::WORD_BIT_COUNT - 1);

  if ((Value & SignBit) == 0) {
    return Value > Maximum ? Maximum : Value;
  }

  return Value < Minimum ? Minimum : Value;
}

inline uint32_t rotateRight(uint32_t Value, uint32_t Shift) {
  if (Shift == 0) {
    return Value;
  }
  return (Value >> Shift) | (Value << (TI32::WORD_BIT_COUNT - Shift));
}

inline uint32_t compressBits(uint32_t Value, uint32_t Mask) {
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

} // namespace ExecutorDetail

inline void executeLi(ExecutionContext &Context,
                      const TIInstruction &Instruction) {
  TICpuState &Cpu                  = Context.cpu();
  const auto &Operands             = Instruction.Operands;
  Cpu.Registers[Operands[0].Value] = Operands[1].Value;
  Cpu.PC += TI32::INSTRUCTION_SIZE;
}

inline void executeAdd(ExecutionContext &Context,
                       const TIInstruction &Instruction) {
  TICpuState &Cpu      = Context.cpu();
  const auto &Operands = Instruction.Operands;
  auto &Registers      = Cpu.Registers;
  Registers[Operands[0].Value] =
      Registers[Operands[1].Value] + Registers[Operands[2].Value];
  Cpu.PC += TI32::INSTRUCTION_SIZE;
}

inline void executeAddi(ExecutionContext &Context,
                        const TIInstruction &Instruction) {
  TICpuState &Cpu      = Context.cpu();
  const auto &Operands = Instruction.Operands;
  auto &Registers      = Cpu.Registers;
  Registers[Operands[0].Value] =
      Registers[Operands[1].Value] + Operands[2].Value;
  Cpu.PC += TI32::INSTRUCTION_SIZE;
}

inline void executeOr(ExecutionContext &Context,
                      const TIInstruction &Instruction) {
  TICpuState &Cpu      = Context.cpu();
  const auto &Operands = Instruction.Operands;
  auto &Registers      = Cpu.Registers;
  Registers[Operands[0].Value] =
      Registers[Operands[1].Value] | Registers[Operands[2].Value];
  Cpu.PC += TI32::INSTRUCTION_SIZE;
}

inline void executeLdReg(ExecutionContext &Context,
                         const TIInstruction &Instruction) {
  TICpuState &Cpu      = Context.cpu();
  TIMemory &Memory     = Context.memory();
  const auto &Operands = Instruction.Operands;
  auto &Registers      = Cpu.Registers;
  uint32_t Address =
      Registers[Operands[1].Value] + Registers[Operands[2].Value];
  Registers[Operands[0].Value] = Memory.read32(Address);
  Cpu.PC += TI32::INSTRUCTION_SIZE;
}

inline void executeLdImm(ExecutionContext &Context,
                         const TIInstruction &Instruction) {
  TICpuState &Cpu      = Context.cpu();
  TIMemory &Memory     = Context.memory();
  const auto &Operands = Instruction.Operands;
  auto &Registers      = Cpu.Registers;
  uint32_t Address     = Registers[Operands[1].Value] + Operands[2].Value;
  Registers[Operands[0].Value] = Memory.read32(Address);
  Cpu.PC += TI32::INSTRUCTION_SIZE;
}

inline void executeSt(ExecutionContext &Context,
                      const TIInstruction &Instruction) {
  TICpuState &Cpu      = Context.cpu();
  TIMemory &Memory     = Context.memory();
  const auto &Operands = Instruction.Operands;
  auto &Registers      = Cpu.Registers;
  uint32_t Address     = Registers[Operands[1].Value] + Operands[2].Value;
  Memory.write32(Address, Registers[Operands[0].Value]);
  Cpu.PC += TI32::INSTRUCTION_SIZE;
}

inline void executeStSmc(ExecutionContext &Context,
                         const TIInstruction &Instruction) {
  TICpuState &Cpu       = Context.cpu();
  const auto &Operands  = Instruction.Operands;
  const auto &Registers = Cpu.Registers;
  uint32_t Address      = Registers[Operands[1].Value] + Operands[2].Value;

  executeSt(Context, Instruction);
  Context.invalidateCodeRange(Address, TI32::WORD_SIZE);
}

inline void executeStp(ExecutionContext &Context,
                       const TIInstruction &Instruction) {
  TICpuState &Cpu      = Context.cpu();
  TIMemory &Memory     = Context.memory();
  const auto &Operands = Instruction.Operands;
  auto &Registers      = Cpu.Registers;
  uint32_t Address     = Registers[Operands[2].Value] + Operands[3].Value;

  Memory.write32(Address, Registers[Operands[0].Value]);
  Memory.write32(Address + TI32::WORD_SIZE, Registers[Operands[1].Value]);
  Cpu.PC += TI32::INSTRUCTION_SIZE;
}

inline void executeStpSmc(ExecutionContext &Context,
                          const TIInstruction &Instruction) {
  TICpuState &Cpu       = Context.cpu();
  const auto &Operands  = Instruction.Operands;
  const auto &Registers = Cpu.Registers;
  uint32_t Address      = Registers[Operands[2].Value] + Operands[3].Value;

  executeStp(Context, Instruction);
  Context.invalidateCodeRange(Address, 2 * TI32::WORD_SIZE);
}

inline void executeBeq(ExecutionContext &Context,
                       const TIInstruction &Instruction) {
  TICpuState &Cpu      = Context.cpu();
  const auto &Operands = Instruction.Operands;
  auto &Registers      = Cpu.Registers;
  if (Registers[Operands[0].Value] == Registers[Operands[1].Value]) {
    Cpu.PC += Operands[2].Value * TI32::INSTRUCTION_SIZE;
  } else {
    Cpu.PC += TI32::INSTRUCTION_SIZE;
  }
}

inline void executeJ(ExecutionContext &Context,
                     const TIInstruction &Instruction) {
  TICpuState &Cpu      = Context.cpu();
  const auto &Operands = Instruction.Operands;
  Cpu.PC = (Cpu.PC & 0xF0000000) | (Operands[0].Value * TI32::INSTRUCTION_SIZE);
}

inline void executeClz(ExecutionContext &Context,
                       const TIInstruction &Instruction) {
  TICpuState &Cpu      = Context.cpu();
  const auto &Operands = Instruction.Operands;
  auto &Registers      = Cpu.Registers;
  Registers[Operands[0].Value] =
      ExecutorDetail::countLeadingZeros(Registers[Operands[1].Value]);
  Cpu.PC += TI32::INSTRUCTION_SIZE;
}

inline void executeSsat(ExecutionContext &Context,
                        const TIInstruction &Instruction) {
  TICpuState &Cpu              = Context.cpu();
  const auto &Operands         = Instruction.Operands;
  auto &Registers              = Cpu.Registers;
  Registers[Operands[0].Value] = ExecutorDetail::saturateSigned(
      Registers[Operands[1].Value], Operands[2].Value);
  Cpu.PC += TI32::INSTRUCTION_SIZE;
}

inline void executeRori(ExecutionContext &Context,
                        const TIInstruction &Instruction) {
  TICpuState &Cpu              = Context.cpu();
  const auto &Operands         = Instruction.Operands;
  auto &Registers              = Cpu.Registers;
  Registers[Operands[0].Value] = ExecutorDetail::rotateRight(
      Registers[Operands[1].Value], Operands[2].Value);
  Cpu.PC += TI32::INSTRUCTION_SIZE;
}

inline void executeBext(ExecutionContext &Context,
                        const TIInstruction &Instruction) {
  TICpuState &Cpu              = Context.cpu();
  const auto &Operands         = Instruction.Operands;
  auto &Registers              = Cpu.Registers;
  Registers[Operands[0].Value] = ExecutorDetail::compressBits(
      Registers[Operands[1].Value], Registers[Operands[2].Value]);
  Cpu.PC += TI32::INSTRUCTION_SIZE;
}

inline void executeSyscall(ExecutionContext &Context, const TIInstruction &) {
  Context.syscallEmulator().execute(Context.cpu(), Context.memory(),
                                    Context.state());
  if (Context.state().Status == ExecutionStatus::RUNNING) {
    Context.cpu().PC += TI32::INSTRUCTION_SIZE;
  }
}

inline void executeInvalid(ExecutionContext &, const TIInstruction &) {
  throw SimulationException(ErrorCode::INVALID_INSTRUCTION,
                            "[EXECUTE] Invalid instruction");
}
