#pragma once

#include "instruction.hpp"

#include <array>
#include <cstdint>

class TIDecoder {
public:
  TIInstruction decode(uint32_t Word) const;

private:
  struct DecodeInfo final {
    TIOpcode OpCode    = TIOpcode::INVALID;
    uint32_t Mask      = 0x00000000;
    uint32_t Reference = 0x00000000;

    TIInstruction (*Handler)(uint32_t) = nullptr;
  };

  // Match all fixed bits: (Word & Mask) == Reference.
  static const std::array<DecodeInfo, 16> DecodeVector;
};
