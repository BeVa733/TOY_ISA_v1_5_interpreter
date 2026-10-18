#include "../../include/execution_type/jit_executor.hpp"
#include "../../include/toy/basic_block_cache.hpp"
#include "../../include/toy/execution_context.hpp"
#include "../../include/toy/executor.hpp"

#include <ir.h>
#include <ir_builder.h>

#include <cstddef>
#include <stdexcept>

namespace {

using TIJitBlock           = void (*)(ExecutionContext *);
using TIInstructionHandler = void (*)(ExecutionContext &,
                                      const TIInstruction &);

void encodeCall(ir_ctx *ctx, ir_ref ContextReference,
                const TIInstruction &Instruction,
                TIInstructionHandler Handler) {
  ir_ref HandlerReference     = ir_CONST_ADDR(Handler);
  ir_ref InstructionReference = ir_CONST_ADDR(&Instruction);
  ir_CALL_2(IR_VOID, HandlerReference, ContextReference, InstructionReference);
}

void encodeInstruction(ir_ctx *ctx, ir_ref ContextReference,
                       const TIInstruction &Instruction) {
  switch (Instruction.OpCode) {
  case TIOpcode::LI:
    return encodeCall(ctx, ContextReference, Instruction, executeLi);
  case TIOpcode::ADD:
    return encodeCall(ctx, ContextReference, Instruction, executeAdd);
  case TIOpcode::ADDI:
    return encodeCall(ctx, ContextReference, Instruction, executeAddi);
  case TIOpcode::OR:
    return encodeCall(ctx, ContextReference, Instruction, executeOr);
  case TIOpcode::LDreg:
    return encodeCall(ctx, ContextReference, Instruction, executeLdReg);
  case TIOpcode::ST:
    return encodeCall(ctx, ContextReference, Instruction, executeSt);
  case TIOpcode::STP:
    return encodeCall(ctx, ContextReference, Instruction, executeStp);
  case TIOpcode::BEQ:
    return encodeCall(ctx, ContextReference, Instruction, executeBeq);
  case TIOpcode::J:
    return encodeCall(ctx, ContextReference, Instruction, executeJ);
  case TIOpcode::CLZ:
    return encodeCall(ctx, ContextReference, Instruction, executeClz);
  case TIOpcode::SSAT:
    return encodeCall(ctx, ContextReference, Instruction, executeSsat);
  case TIOpcode::RORI:
    return encodeCall(ctx, ContextReference, Instruction, executeRori);
  case TIOpcode::BEXT:
    return encodeCall(ctx, ContextReference, Instruction, executeBext);
  case TIOpcode::SYSCALL:
    return encodeCall(ctx, ContextReference, Instruction, executeSyscall);
  case TIOpcode::LDimm:
    return encodeCall(ctx, ContextReference, Instruction, executeLdImm);
  case TIOpcode::INVALID:
  default:
    return encodeCall(ctx, ContextReference, Instruction, executeInvalid);
  }
}

TIJitBlock encodeBlock(ir_ctx *ctx, const TIBasicBlock &Block,
                       std::size_t &CodeSize) {
  ir_START();
  ir_ref ContextReference = ir_PARAM(IR_ADDR, "context", 1);

  for (const TIInstruction &Instruction : Block.instructions()) {
    encodeInstruction(ctx, ContextReference, Instruction);
  }

  ir_RETURN(IR_UNUSED);
  return reinterpret_cast<TIJitBlock>(ir_jit_compile(ctx, 0, &CodeSize));
}

} // namespace

void executeJitBlock(ExecutionContext &Context, const TIBasicBlock &Block) {
  ir_ctx IrContext{};
  ir_init(&IrContext, IR_FUNCTION, IR_CONSTS_LIMIT_MIN, IR_INSNS_LIMIT_MIN);

  std::size_t CodeSize = 0;
  TIJitBlock JitBlock  = encodeBlock(&IrContext, Block, CodeSize);
  if (JitBlock == nullptr) {
    ir_free(&IrContext);
    throw std::runtime_error("IR failed to compile a basic block");
  }

  JitBlock(&Context);

  ir_mem_unmap(reinterpret_cast<void *>(JitBlock), CodeSize);
  ir_free(&IrContext);
}
