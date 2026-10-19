#pragma once

#include "../execution_type/jit_executor.hpp"
#include "decoder.hpp"
#include "memory.hpp"

#include <cstddef>
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

  uint32_t endAddress() const {
    return StartAddress_ +
           static_cast<uint32_t>(Instructions.size() * TI32::INSTRUCTION_SIZE);
  }

  std::size_t instructionCount() const { return Instructions.size(); }

  uint64_t executionCount() const { return ExecutionCount_; }

  void recordExecution() { ++ExecutionCount_; }

  const std::vector<TIInstruction> &instructions() const {
    return Instructions;
  }

  bool hasCompiledBlock() const { return !CompiledBlock.empty(); }

  TICompiledBlock &compiledBlock() { return CompiledBlock; }

  void setCompiledBlock(TICompiledBlock Block) {
    CompiledBlock = std::move(Block);
  }

private:
  uint32_t StartAddress_   = 0;
  uint64_t ExecutionCount_ = 0;
  std::vector<TIInstruction> Instructions{};
  TICompiledBlock CompiledBlock{};
};

class TIBasicBlockCache final {
public:
  /// Find a block entry by \p Address or decode a new block.
  TIBasicBlock &getBlock(uint32_t Address, const TIMemory &Memory,
                         TIDecoder &Decoder);

  /// Return a stable slot containing the translated entry for an address.
  TIJitFunction *jitEntrySlot(uint32_t Address) {
    return &JitEntries.try_emplace(Address, nullptr).first->second;
  }

  /// Publish a translated entry point for block chaining.
  void setJitEntry(uint32_t Address, TIJitFunction EntryPoint) {
    *jitEntrySlot(Address) = EntryPoint;
  }

  void clear() {
    Blocks.clear();
    JitEntries.clear();
  }

  void invalidateRange(uint32_t Address, std::size_t ByteCount);

private:
  using BlockMap    = std::unordered_map<uint32_t, TIBasicBlock>;
  using JitEntryMap = std::unordered_map<uint32_t, TIJitFunction>;

  BlockMap Blocks{};
  // Entries are not erased individually because translated blocks embed their
  // addresses. Invalidation only clears their values.
  JitEntryMap JitEntries{};

  TIBasicBlock &decodeBlock(uint32_t Address, const TIMemory &Memory,
                            TIDecoder &Decoder);

  static bool endsBlock(TIOpcode OpCode);
};
