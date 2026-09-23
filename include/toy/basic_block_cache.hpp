#pragma once

#include "decoder.hpp"
#include "memory.hpp"

#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

class TIBasicBlock final {
public:
  TIBasicBlock(uint32_t StartAddress,
               std::vector<TIInstruction> DecodedInstructions)
      : StartAddress_(StartAddress),
        Instructions(std::move(DecodedInstructions)) {}

public:
  /// State geters 
  uint32_t startAddress() const { return StartAddress_; }

  const std::vector<TIInstruction> &instructions() const {
    return Instructions;
  }

private:
  uint32_t StartAddress_ = 0;
  std::vector<TIInstruction> Instructions{};

};

class TIBasicBlockCache final {
public:
  /// Find a block entry by \p Address or decode a new block.
  TIBasicBlock &getBlock(uint32_t Address, const TIMemory &Memory,
                         TIDecoder &Decoder);

  void clear() { Blocks.clear(); }

private:
  using BlockMap = std::unordered_map<uint32_t, TIBasicBlock>;

  BlockMap Blocks{};

  TIBasicBlock &decodeBlock(uint32_t Address, const TIMemory &Memory,
                            TIDecoder &Decoder);

  static bool endsBlock(TIOpcode OpCode);
};
