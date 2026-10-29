#include "toy/decoder.hpp"
#include "toy/bitutils.hpp"   

#include "execution_type/threaded_executor.hpp"

namespace {

#define DEFINE_DECODE_FUNCTION(Name, ...)                                      \
  TIInstruction decode##Name(uint32_t Word) { __VA_ARGS__ }

DEFINE_DECODE_FUNCTION(St, {
  uint32_t Base      = extractBits(Word, 21, 5);
  uint32_t Rt        = extractBits(Word, 16, 5);
  uint32_t Immediate = signExtend(extractBits(Word, 0, 14), 14);
  return {
      TIOpcode::ST,
      {{OperandType::REGISTER, Rt},
            {OperandType::REGISTER, Base},
            {OperandType::IMMEDIATE, Immediate}}
  };
})

DEFINE_DECODE_FUNCTION(StSmc, {
  uint32_t Base      = extractBits(Word, 21, 5);
  uint32_t Rt        = extractBits(Word, 16, 5);
  uint32_t Immediate = signExtend(extractBits(Word, 0, 14), 14);
  return {
      TIOpcode::ST_SMC,
      {{OperandType::REGISTER, Rt},
                {OperandType::REGISTER, Base},
                {OperandType::IMMEDIATE, Immediate}}
  };
})

DEFINE_DECODE_FUNCTION(Addi, {
  uint32_t Rs        = extractBits(Word, 21, 5);
  uint32_t Rt        = extractBits(Word, 16, 5);
  uint32_t Immediate = signExtend(extractBits(Word, 0, 16), 16);
  return {
      TIOpcode::ADDI,
      {{OperandType::REGISTER, Rt},
              {OperandType::REGISTER, Rs},
              {OperandType::IMMEDIATE, Immediate}}
  };
})

DEFINE_DECODE_FUNCTION(Or, {
  uint32_t Rs = extractBits(Word, 21, 5);
  uint32_t Rt = extractBits(Word, 16, 5);
  uint32_t Rd = extractBits(Word, 11, 5);
  return {
      TIOpcode::OR,
      {{OperandType::REGISTER, Rd},
            {OperandType::REGISTER, Rs},
            {OperandType::REGISTER, Rt}}
  };
})

DEFINE_DECODE_FUNCTION(LdReg, {
  uint32_t Base = extractBits(Word, 21, 5);
  uint32_t Rt   = extractBits(Word, 16, 5);
  uint32_t Rm   = extractBits(Word, 0, 5);
  return {
      TIOpcode::LDreg,
      {{OperandType::REGISTER, Rt},
               {OperandType::REGISTER, Base},
               {OperandType::REGISTER, Rm}}
  };
})

DEFINE_DECODE_FUNCTION(J, {
  return {TIOpcode::J, {{OperandType::IMMEDIATE, extractBits(Word, 0, 26)}}};
})

DEFINE_DECODE_FUNCTION(Beq, {
  uint32_t Rs     = extractBits(Word, 21, 5);
  uint32_t Rt     = extractBits(Word, 16, 5);
  uint32_t Offset = signExtend(extractBits(Word, 0, 16), 16);
  return {
      TIOpcode::BEQ,
      {{OperandType::REGISTER, Rs},
             {OperandType::REGISTER, Rt},
             {OperandType::IMMEDIATE, Offset}}
  };
})

DEFINE_DECODE_FUNCTION(Clz, {
  return {
      TIOpcode::CLZ,
      {{OperandType::REGISTER, extractBits(Word, 21, 5)},
             {OperandType::REGISTER, extractBits(Word, 16, 5)}}
  };
})

DEFINE_DECODE_FUNCTION(Ssat, {
  return {
      TIOpcode::SSAT,
      {{OperandType::REGISTER, extractBits(Word, 21, 5)},
              {OperandType::REGISTER, extractBits(Word, 16, 5)},
              {OperandType::IMMEDIATE, extractBits(Word, 11, 5)}}
  };
})

DEFINE_DECODE_FUNCTION(LdImm, {
  uint32_t Base      = extractBits(Word, 21, 5);
  uint32_t Rt        = extractBits(Word, 16, 5);
  uint32_t Immediate = signExtend(extractBits(Word, 0, 14), 14);
  return {
      TIOpcode::LDimm,
      {{OperandType::REGISTER, Rt},
               {OperandType::REGISTER, Base},
               {OperandType::IMMEDIATE, Immediate}}
  };
})

DEFINE_DECODE_FUNCTION(Add, {
  return {
      TIOpcode::ADD,
      {{OperandType::REGISTER, extractBits(Word, 11, 5)},
             {OperandType::REGISTER, extractBits(Word, 21, 5)},
             {OperandType::REGISTER, extractBits(Word, 16, 5)}}
  };
})

DEFINE_DECODE_FUNCTION(Syscall, {
  return {TIOpcode::SYSCALL,
          {{OperandType::IMMEDIATE, extractBits(Word, 6, 20)}}};
})

DEFINE_DECODE_FUNCTION(Bext, {
  return {
      TIOpcode::BEXT,
      {{OperandType::REGISTER, extractBits(Word, 21, 5)},
              {OperandType::REGISTER, extractBits(Word, 16, 5)},
              {OperandType::REGISTER, extractBits(Word, 11, 5)}}
  };
})

DEFINE_DECODE_FUNCTION(Li, {
  return {
      TIOpcode::LI,
      {{OperandType::REGISTER, extractBits(Word, 16, 5)},
            {OperandType::IMMEDIATE, signExtend(extractBits(Word, 0, 16), 16)}}
  };
})

DEFINE_DECODE_FUNCTION(Rori, {
  return {
      TIOpcode::RORI,
      {{OperandType::REGISTER, extractBits(Word, 21, 5)},
              {OperandType::REGISTER, extractBits(Word, 16, 5)},
              {OperandType::IMMEDIATE, extractBits(Word, 11, 5)}}
  };
})

DEFINE_DECODE_FUNCTION(Stp, {
  return {
      TIOpcode::STP,
      {{OperandType::REGISTER, extractBits(Word, 16, 5)},
             {OperandType::REGISTER, extractBits(Word, 11, 5)},
             {OperandType::REGISTER, extractBits(Word, 21, 5)},
             {OperandType::IMMEDIATE, signExtend(extractBits(Word, 0, 11), 11)}}
  };
})

DEFINE_DECODE_FUNCTION(StpSmc, {
  return {
      TIOpcode::STP_SMC,
      {{OperandType::REGISTER, extractBits(Word, 16, 5)},
                 {OperandType::REGISTER, extractBits(Word, 11, 5)},
                 {OperandType::REGISTER, extractBits(Word, 21, 5)},
                 {OperandType::IMMEDIATE, signExtend(extractBits(Word, 0, 11), 11)}}
  };
})

#undef DEFINE_DECODE_FUNCTION

} // namespace

const std::array<TIDecoder::DecodeInfo, 17> TIDecoder::DecodeVector{
    {{TIOpcode::ST, 0xFC00C000, 0x80000000, decodeSt},
     {TIOpcode::ST_SMC, 0xFC000000, 0x90000000, decodeStSmc},
     {TIOpcode::ADDI, 0xFC000000, 0x5C000000, decodeAddi},
     {TIOpcode::OR, 0xFC0007FF, 0x0000001A, decodeOr},
     {TIOpcode::LDreg, 0xFC00FFE0, 0x0C00C000, decodeLdReg},
     {TIOpcode::J, 0xFC000000, 0xFC000000, decodeJ},
     {TIOpcode::BEQ, 0xFC000000, 0x40000000, decodeBeq},
     {TIOpcode::CLZ, 0xFC00FFFF, 0x00000032, decodeClz},
     {TIOpcode::SSAT, 0xFC0007FF, 0x20000000, decodeSsat},
     {TIOpcode::LDimm, 0xFC00C000, 0xCC000000, decodeLdImm},
     {TIOpcode::ADD, 0xFC0007FF, 0x0000000C, decodeAdd},
     {TIOpcode::SYSCALL, 0xFC00003F, 0x0000001E, decodeSyscall},
     {TIOpcode::BEXT, 0xFC0007FF, 0x00000034, decodeBext},
     {TIOpcode::LI, 0xFFE00000, 0x2C000000, decodeLi},
     {TIOpcode::RORI, 0xFC0007FF, 0x74000000, decodeRori},
     {TIOpcode::STP, 0xFC000000, 0x54000000, decodeStp},
     {TIOpcode::STP_SMC, 0xFC000000, 0x94000000, decodeStpSmc}}
};

TIInstruction TIDecoder::decode(uint32_t Word) const {
  for (const DecodeInfo &Info : DecodeVector) {
    if ((Word & Info.Mask) == Info.Reference) {
      TIInstruction Instruction   = Info.Handler(Word);
      Instruction.ThreadedHandler = getThreadedHandler(Instruction.OpCode);
      return Instruction;
    }
  }

  return {TIOpcode::INVALID, {}, getThreadedHandler(TIOpcode::INVALID)};
}
