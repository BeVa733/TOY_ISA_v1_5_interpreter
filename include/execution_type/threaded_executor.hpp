#pragma once

#include "../toy/instruction.hpp"

class TIBasicBlock;
class ExecutionContext;

struct TIThreadState final {
  TIThreadState(ExecutionContext &Execution,
                const TIInstruction *FirstInstruction,
                const TIInstruction *EndInstruction)
      : Context(Execution), Current(FirstInstruction), End(EndInstruction) {}

  ExecutionContext &Context;
  const TIInstruction *Current = nullptr;
  const TIInstruction *End     = nullptr;
};

TIThreadedHandler getThreadedHandler(TIOpcode OpCode);

void executeThreadedBlock(ExecutionContext &Context, const TIBasicBlock &Block);
