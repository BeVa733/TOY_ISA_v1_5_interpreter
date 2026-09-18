#pragma once

#include <cstdint>
#include <vector>

// Internal operation identifiers, not ISA bit encodings.
enum class Opcode : uint8_t {
  INVALID = 0,
  LI      = 1,
  ADD     = 2,
  ADDI    = 3,
  OR      = 4,
  LD      = 5,
  ST      = 6,
  STP     = 7,
  BEQ     = 8,
  J       = 9,
  CLZ     = 10,
  SSAT    = 11,
  RORI    = 12,
  BEXT    = 13,
  SYSCALL = 14
};

enum class OperandType : uint8_t {
  REGISTER  = 0,
  IMMEDIATE = 1
};

struct Operand {
  OperandType Type = OperandType::IMMEDIATE;
  uint32_t Value   = 0;
};

struct Instruction {
  Opcode OpCode = Opcode::INVALID;
  std::vector<Operand> Operands{};
};
