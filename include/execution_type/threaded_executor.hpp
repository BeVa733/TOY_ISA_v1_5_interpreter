#pragma once

#include "toy/instruction.hpp"

class TIBasicBlock;
class ExecutionContext;

TIThreadedHandler getThreadedHandler(TIOpcode OpCode);

void executeThreadedBlock(ExecutionContext &Context, const TIBasicBlock &Block);
