#include "../../include/toy/execution_context.hpp"

namespace {

uint32_t countLeadingZeros(uint32_t Value) {
  uint32_t Count = 0;
  for (uint32_t Mask = 0x80000000; Mask != 0 && (Value & Mask) == 0;
       Mask >>= 1) {
    ++Count;
  }
  return Count;
}

uint32_t saturateSigned(uint32_t Value, uint32_t Width) {
  if (Width == 0 || Width > 31) {
    throw SimulationException(ErrorCode::INVALID_INSTRUCTION,
                              "[EXECUTE] SSAT width must be in range 1..31");
  }

  int64_t SignedValue = Value;
  if ((Value & 0x80000000) != 0) {
    SignedValue -= int64_t{1} << 32;
  }

  int64_t Minimum = -(int64_t{1} << (Width - 1));
  int64_t Maximum = (int64_t{1} << (Width - 1)) - 1;
  if (SignedValue < Minimum) {
    SignedValue = Minimum;
  } else if (SignedValue > Maximum) {
    SignedValue = Maximum;
  }
  return static_cast<uint32_t>(SignedValue);
}

uint32_t rotateRight(uint32_t Value, uint32_t Shift) {
  if (Shift == 0) {
    return Value;
  }
  return (Value >> Shift) | (Value << (32 - Shift));
}

uint32_t extractBits(uint32_t Value, uint32_t Mask) {
  uint32_t Result    = 0;
  uint32_t OutputBit = 0;
  for (uint32_t InputBit = 0; InputBit < 32; ++InputBit) {
    if ((Mask & (uint32_t{1} << InputBit)) != 0) {
      Result |= ((Value >> InputBit) & 0x01) << OutputBit;
      ++OutputBit;
    }
  }
  return Result;
}

} // namespace

void ExecutionContext::execute(const TIInstruction &Inst) {
  // Operand layouts and immediate sign extension are provided by TIDecoder.
  const auto &Operands = Inst.Operands;
  auto &Registers      = Cpu.Registers;
  uint32_t NextPC      = Cpu.PC + uint32_t{4};

  try {
    switch (Inst.OpCode) {
    case TIOpcode::LI:
      Registers[Operands[0].Value] = Operands[1].Value;
      break;

    case TIOpcode::ADD:
      Registers[Operands[0].Value] =
          Registers[Operands[1].Value] + Registers[Operands[2].Value];
      break;

    case TIOpcode::ADDI:
      Registers[Operands[0].Value] =
          Registers[Operands[1].Value] + Operands[2].Value;
      break;

    case TIOpcode::OR:
      Registers[Operands[0].Value] =
          Registers[Operands[1].Value] | Registers[Operands[2].Value];
      break;

    case TIOpcode::LDreg: {
      uint32_t Address =
          Registers[Operands[1].Value] + Registers[Operands[2].Value];
      Registers[Operands[0].Value] = Memory.read32(Address);
      break;
    }

    case TIOpcode::LDimm: {
      uint32_t Address = Registers[Operands[1].Value] + Operands[2].Value;
      Registers[Operands[0].Value] = Memory.read32(Address);
      break;
    }

    case TIOpcode::ST: {
      uint32_t Address = Registers[Operands[1].Value] + Operands[2].Value;
      Memory.write32(Address, Registers[Operands[0].Value]);
      break;
    }

    case TIOpcode::STP: {
      uint32_t Address = Registers[Operands[2].Value] + Operands[3].Value;
      Memory.writePair32(Address, Registers[Operands[0].Value],
                         Registers[Operands[1].Value]);
      break;
    }

    case TIOpcode::BEQ:
      if (Registers[Operands[0].Value] == Registers[Operands[1].Value]) {
        NextPC = Cpu.PC + (Operands[2].Value << 2);
      }
      break;

    case TIOpcode::J:
      NextPC = (Cpu.PC & 0xF0000000) | (Operands[0].Value << 2);
      break;

    case TIOpcode::CLZ:
      Registers[Operands[0].Value] =
          countLeadingZeros(Registers[Operands[1].Value]);
      break;

    case TIOpcode::SSAT:
      Registers[Operands[0].Value] =
          saturateSigned(Registers[Operands[1].Value], Operands[2].Value);
      break;

    case TIOpcode::RORI:
      Registers[Operands[0].Value] =
          rotateRight(Registers[Operands[1].Value], Operands[2].Value);
      break;

    case TIOpcode::BEXT:
      Registers[Operands[0].Value] = extractBits(Registers[Operands[1].Value],
                                                 Registers[Operands[2].Value]);
      break;

    case TIOpcode::SYSCALL: {
      constexpr uint32_t EXIT_SYSCALL = 93;
      if (Registers[8] != EXIT_SYSCALL) {
        throw SimulationException(ErrorCode::UNSUPPORTED_SYSCALL,
                                  "[EXECUTE] Unsupported syscall: " +
                                      std::to_string(Registers[8]));
      }
      State.halt(Registers[0]);
      return;
    }

    case TIOpcode::INVALID:
    default:
      throw SimulationException(ErrorCode::INVALID_INSTRUCTION,
                                "[EXECUTE] Invalid instruction");
    }
  } catch (const SimulationException &) {
    State.fault();
    throw;
  }

  Cpu.PC = NextPC;
}
