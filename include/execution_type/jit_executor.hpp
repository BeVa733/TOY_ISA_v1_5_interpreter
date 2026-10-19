#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>

class TIBasicBlock;
class ExecutionContext;

using TIJitFunction = void (*)(ExecutionContext *);

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

  void execute(ExecutionContext &Context) const { EntryPoint_(&Context); }

private:
  TIJitFunction EntryPoint_ = nullptr;
  std::size_t CodeSize_     = 0;

  void reset();
};

/// Compile and execute one basic block through IR.
void executeJitBlock(ExecutionContext &Context, TIBasicBlock &Block,
                     uint64_t Threshold);
