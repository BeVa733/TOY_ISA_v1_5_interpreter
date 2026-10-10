#include "../../include/toy/basic_block_cache.hpp"
#include "../../include/toy/toy_constants.hpp"

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

    CurrentAddress += TI32::INSTRUCTION_SIZE;

    if (static_cast<uint64_t>(CurrentAddress) + TI32::INSTRUCTION_SIZE >
        Memory.size()) {
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

void TIBasicBlockCache::invalidateRange(uint32_t Address,
                                        std::size_t ByteCount) {
  const uint64_t WriteBegin = Address;
  const uint64_t WriteEnd   = WriteBegin + ByteCount;

  for (auto It = Blocks.begin(); It != Blocks.end();) {
    const uint64_t BlockBegin = It->second.startAddress();
    const uint64_t BlockEnd   = It->second.endAddress();
    if (BlockBegin < WriteEnd && WriteBegin < BlockEnd) {
      It = Blocks.erase(It);
    } else {
      ++It;
    }
  }
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
