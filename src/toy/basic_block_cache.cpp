#include "../../include/toy/basic_block_cache.hpp"

#include <utility>

TIBasicBlock &TIBasicBlockCache::decodeBlock(uint32_t Address,
                                             const TIMemory &Memory,
                                             TIDecoder &Decoder) {
  std::vector<TIInstruction> Instructions{};
  uint32_t CurrentAddress = Address;

  while (true) {
    uint32_t Encoding         = Memory.read32(CurrentAddress);
    TIInstruction Instruction = Decoder.decode(Encoding);
    Instructions.push_back(std::move(Instruction));

    if (endsBlock(Instructions.back().OpCode)) {
      break;
    }

    CurrentAddress += 4;

    if (Memory.hasInstructionRange() &&
        !Memory.containsInstruction(CurrentAddress)) {
      break;
    }

    if (static_cast<uint64_t>(CurrentAddress) + 4 > Memory.size()) {
      break;
    }
  }

  auto [BlockIt, Inserted] =
      Blocks.emplace(Address, TIBasicBlock(Address, std::move(Instructions)));
  (void)Inserted;
  return BlockIt->second;
}

TIBasicBlock &TIBasicBlockCache::getBlock(uint32_t Address,
                                          const TIMemory &Memory,
                                          TIDecoder &Decoder) {
  auto It = Blocks.find(Address);
  if (It != Blocks.end()) {
    return It->second;
  }

  return decodeBlock(Address, Memory, Decoder);
}

bool TIBasicBlockCache::endsBlock(TIOpcode OpCode) {
  switch (OpCode) {
  case TIOpcode::BEQ:
  case TIOpcode::J:
  case TIOpcode::SYSCALL:
  case TIOpcode::INVALID:
    return true;

  default:
    return false;
  }
}
