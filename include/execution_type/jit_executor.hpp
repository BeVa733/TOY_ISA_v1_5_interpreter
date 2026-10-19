#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>

class TIBasicBlock;
class TIBasicBlockCache;
class ExecutionContext;

using TIJitFunction = uintptr_t (*)(ExecutionContext *);

class TICompiledBlock final {
public:
  TICompiledBlock() = default;
  TICompiledBlock(TIJitFunction EntryPoint, std::size_t CodeSize)
      : EntryPoint_(EntryPoint), CodeSize_(CodeSize) {}

  // ---------------- Big five --------------------

  TICompiledBlock(TICompiledBlock &&Other) noexcept
      : EntryPoint_(std::exchange(Other.EntryPoint_, nullptr)),
        CodeSize_(std::exchange(Other.CodeSize_, 0)) {}

  TICompiledBlock &operator=(TICompiledBlock &&Other) noexcept {
    if (this != &Other) {
      reset();
      EntryPoint_ = std::exchange(Other.EntryPoint_, nullptr);
      CodeSize_   = std::exchange(Other.CodeSize_, 0);
    }
    return *this;
  }

  TICompiledBlock(const TICompiledBlock &)            = delete;
  TICompiledBlock &operator=(const TICompiledBlock &) = delete;

  ~TICompiledBlock() { reset(); }

  //-----------------------------------------------

  bool empty() const { return EntryPoint_ == nullptr; }

  TIJitFunction execute(ExecutionContext &Context) const {
    return reinterpret_cast<TIJitFunction>(EntryPoint_(&Context));
  }

  /// Return the native entry point stored by this translation.
  TIJitFunction entryPoint() const { return EntryPoint_; }

private:
  TIJitFunction EntryPoint_ = nullptr;
  std::size_t CodeSize_     = 0;

  void reset();
};

/// Compile and execute one basic block through IR.
void executeJitBlock(ExecutionContext &Context, TIBasicBlockCache &Cache,
                     TIBasicBlock &Block, uint64_t Threshold);
