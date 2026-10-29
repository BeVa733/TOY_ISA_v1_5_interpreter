#include "execution_type/switch_executor.hpp"
#include "toy/basic_block_cache.hpp"
#include "toy/execution_context.hpp"
#include "toy/executor.hpp"

namespace {

void executeInstructionSwitch(ExecutionContext &Context,
                              const TIInstruction &Instruction) {
  switch (Instruction.OpCode) {
  case TIOpcode::LI:
    return executeLi(Context, Instruction);
  case TIOpcode::ADD:
    return executeAdd(Context, Instruction);
  case TIOpcode::ADDI:
    return executeAddi(Context, Instruction);
  case TIOpcode::OR:
    return executeOr(Context, Instruction);
  case TIOpcode::LDreg:
    return executeLdReg(Context, Instruction);
  case TIOpcode::ST:
    return executeSt(Context, Instruction);
  case TIOpcode::STP:
    return executeStp(Context, Instruction);
  case TIOpcode::FLUSH:
    return executeFlush(Context, Instruction);
  case TIOpcode::BEQ:
    return executeBeq(Context, Instruction);
  case TIOpcode::J:
    return executeJ(Context, Instruction);
  case TIOpcode::CLZ:
    return executeClz(Context, Instruction);
  case TIOpcode::SSAT:
    return executeSsat(Context, Instruction);
  case TIOpcode::RORI:
    return executeRori(Context, Instruction);
  case TIOpcode::BEXT:
    return executeBext(Context, Instruction);
  case TIOpcode::SYSCALL:
    return executeSyscall(Context, Instruction);
  case TIOpcode::LDimm:
    return executeLdImm(Context, Instruction);
  case TIOpcode::INVALID:
  default:
    return executeInvalid(Context, Instruction);
  }
}

} // namespace

void executeSwitchBlock(ExecutionContext &Context, const TIBasicBlock &Block) {
  const auto &Instructions = Block.instructions();

  for (const TIInstruction &Instruction : Instructions) {
    executeInstructionSwitch(Context, Instruction);
  }
}
