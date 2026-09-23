#pragma once

#include "executor.hpp"

#include <vector>

namespace {

// Decode handlers for each opcode

inline uint32_t signExtendImmediate(uint32_t Value, uint32_t Width) {
  uint32_t SignBit = uint32_t{1} << (Width - 1);
  uint32_t Mask    = (uint32_t{1} << Width) - 1;

  Value &= Mask;
  if ((Value & SignBit) != 0) {
    Value |= ~Mask;
  }

  return Value;
}

inline TIInstruction decodeSt(uint32_t Word) {
  uint32_t Base      = (Word >> 21) & 0x1F;
  uint32_t Rt        = (Word >> 16) & 0x1F;
  uint32_t Immediate = signExtendImmediate(Word & 0x3FFF, 14);
  return {
      TIOpcode::ST,
      {{OperandType::REGISTER, Rt},
            {OperandType::REGISTER, Base},
            {OperandType::IMMEDIATE, Immediate}}
  };
}

inline TIInstruction decodeAddi(uint32_t Word) {
  uint32_t Rs        = (Word >> 21) & 0x1F;
  uint32_t Rt        = (Word >> 16) & 0x1F;
  uint32_t Immediate = signExtendImmediate(Word & 0xFFFF, 16);
  return {
      TIOpcode::ADDI,
      {{OperandType::REGISTER, Rt},
              {OperandType::REGISTER, Rs},
              {OperandType::IMMEDIATE, Immediate}}
  };
}

inline TIInstruction decodeOr(uint32_t Word) {
  uint32_t Rs = (Word >> 21) & 0x1F;
  uint32_t Rt = (Word >> 16) & 0x1F;
  uint32_t Rd = (Word >> 11) & 0x1F;
  return {
      TIOpcode::OR,
      {{OperandType::REGISTER, Rd},
            {OperandType::REGISTER, Rs},
            {OperandType::REGISTER, Rt}}
  };
}

inline TIInstruction decodeLdReg(uint32_t Word) {
  uint32_t Base = (Word >> 21) & 0x1F;
  uint32_t Rt   = (Word >> 16) & 0x1F;
  uint32_t Rm   = Word & 0x1F;
  return {
      TIOpcode::LDreg,
      {{OperandType::REGISTER, Rt},
               {OperandType::REGISTER, Base},
               {OperandType::REGISTER, Rm}}
  };
}

inline TIInstruction decodeJ(uint32_t Word) {
  uint32_t Index = Word & 0x03FFFFFF;
  return {TIOpcode::J, {{OperandType::IMMEDIATE, Index}}};
}

inline TIInstruction decodeBeq(uint32_t Word) {
  uint32_t Rs     = (Word >> 21) & 0x1F;
  uint32_t Rt     = (Word >> 16) & 0x1F;
  uint32_t Offset = signExtendImmediate(Word & 0xFFFF, 16);
  return {
      TIOpcode::BEQ,
      {{OperandType::REGISTER, Rs},
             {OperandType::REGISTER, Rt},
             {OperandType::IMMEDIATE, Offset}}
  };
}

inline TIInstruction decodeClz(uint32_t Word) {
  uint32_t Rd = (Word >> 21) & 0x1F;
  uint32_t Rs = (Word >> 16) & 0x1F;
  return {
      TIOpcode::CLZ,
      {{OperandType::REGISTER, Rd}, {OperandType::REGISTER, Rs}}
  };
}

inline TIInstruction decodeSsat(uint32_t Word) {
  uint32_t Rd        = (Word >> 21) & 0x1F;
  uint32_t Rs        = (Word >> 16) & 0x1F;
  uint32_t Immediate = (Word >> 11) & 0x1F;
  return {
      TIOpcode::SSAT,
      {{OperandType::REGISTER, Rd},
              {OperandType::REGISTER, Rs},
              {OperandType::IMMEDIATE, Immediate}}
  };
}

inline TIInstruction decodeLdImm(uint32_t Word) {
  uint32_t Base      = (Word >> 21) & 0x1F;
  uint32_t Rt        = (Word >> 16) & 0x1F;
  uint32_t Immediate = signExtendImmediate(Word & 0x3FFF, 14);
  return {
      TIOpcode::LDimm,
      {{OperandType::REGISTER, Rt},
               {OperandType::REGISTER, Base},
               {OperandType::IMMEDIATE, Immediate}}
  };
}

inline TIInstruction decodeAdd(uint32_t Word) {
  uint32_t Rs = (Word >> 21) & 0x1F;
  uint32_t Rt = (Word >> 16) & 0x1F;
  uint32_t Rd = (Word >> 11) & 0x1F;
  return {
      TIOpcode::ADD,
      {{OperandType::REGISTER, Rd},
             {OperandType::REGISTER, Rs},
             {OperandType::REGISTER, Rt}}
  };
}

inline TIInstruction decodeSyscall(uint32_t Word) {
  uint32_t Code = (Word >> 6) & 0x000FFFFF;
  return {TIOpcode::SYSCALL, {{OperandType::IMMEDIATE, Code}}};
}

inline TIInstruction decodeBext(uint32_t Word) {
  uint32_t Rd  = (Word >> 21) & 0x1F;
  uint32_t Rs1 = (Word >> 16) & 0x1F;
  uint32_t Rs2 = (Word >> 11) & 0x1F;
  return {
      TIOpcode::BEXT,
      {{OperandType::REGISTER, Rd},
              {OperandType::REGISTER, Rs1},
              {OperandType::REGISTER, Rs2}}
  };
}

inline TIInstruction decodeLi(uint32_t Word) {
  uint32_t Rt        = (Word >> 16) & 0x1F;
  uint32_t Immediate = signExtendImmediate(Word & 0xFFFF, 16);
  return {
      TIOpcode::LI,
      {{OperandType::REGISTER, Rt}, {OperandType::IMMEDIATE, Immediate}}
  };
}

inline TIInstruction decodeRori(uint32_t Word) {
  uint32_t Rd        = (Word >> 21) & 0x1F;
  uint32_t Rs        = (Word >> 16) & 0x1F;
  uint32_t Immediate = (Word >> 11) & 0x1F;
  return {
      TIOpcode::RORI,
      {{OperandType::REGISTER, Rd},
              {OperandType::REGISTER, Rs},
              {OperandType::IMMEDIATE, Immediate}}
  };
}

inline TIInstruction decodeStp(uint32_t Word) {
  uint32_t Base   = (Word >> 21) & 0x1F;
  uint32_t Rt1    = (Word >> 16) & 0x1F;
  uint32_t Rt2    = (Word >> 11) & 0x1F;
  uint32_t Offset = signExtendImmediate(Word & 0x07FF, 11);

  return {
      TIOpcode::STP,
      {{OperandType::REGISTER, Rt1},
             {OperandType::REGISTER, Rt2},
             {OperandType::REGISTER, Base},
             {OperandType::IMMEDIATE, Offset}}
  };
}

} // namespace

class TIDecoder {
public:
  TIInstruction decode(uint32_t Word) {
    for (auto It = DecodeVector.begin(); It != DecodeVector.end(); ++It) {
      if ((Word & It->Mask) == It->Reference) {
        TIInstruction Instruction = It->Handler(Word);
        Instruction.Execute       = getExecuteHandler(Instruction.OpCode);
        return Instruction;
      }
    }

    return {TIOpcode::INVALID, {}, getExecuteHandler(TIOpcode::INVALID)};
  }

private:

  struct DecodeInfo final {
    TIOpcode OpCode    = TIOpcode::INVALID;
    uint32_t Mask      = 0x00000000;
    uint32_t Reference = 0x00000000;

    TIInstruction (*Handler)(uint32_t) = nullptr;
  };

  // Match all fixed bits: (Word & Mask) == Reference.
  const std::vector<DecodeInfo> DecodeVector{
      {TIOpcode::ST,      0xFC00C000, 0x80000000, decodeSt     },
      {TIOpcode::ADDI,    0xFC000000, 0x5C000000, decodeAddi   },
      {TIOpcode::OR,      0xFC0007FF, 0x0000001A, decodeOr     },
      {TIOpcode::LDreg,   0xFC00FFE0, 0x0C00C000, decodeLdReg  },
      {TIOpcode::J,       0xFC000000, 0xFC000000, decodeJ      },
      {TIOpcode::BEQ,     0xFC000000, 0x40000000, decodeBeq    },
      {TIOpcode::CLZ,     0xFC00FFFF, 0x00000032, decodeClz    },
      {TIOpcode::SSAT,    0xFC0007FF, 0x20000000, decodeSsat   },
      {TIOpcode::LDimm,   0xFC00C000, 0xCC000000, decodeLdImm  },
      {TIOpcode::ADD,     0xFC0007FF, 0x0000000C, decodeAdd    },
      {TIOpcode::SYSCALL, 0xFC00003F, 0x0000001E, decodeSyscall},
      {TIOpcode::BEXT,    0xFC0007FF, 0x00000034, decodeBext   },
      {TIOpcode::LI,      0xFFE00000, 0x2C000000, decodeLi     },
      {TIOpcode::RORI,    0xFC0007FF, 0x74000000, decodeRori   },
      {TIOpcode::STP,     0xFC000000, 0x54000000, decodeStp    }
  };
};
