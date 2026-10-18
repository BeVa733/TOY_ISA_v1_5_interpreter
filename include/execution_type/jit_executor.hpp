#pragma once

class TIBasicBlock;
class ExecutionContext;

/// Compile and execute one basic block through IR.
void executeJitBlock(ExecutionContext &Context, const TIBasicBlock &Block);
