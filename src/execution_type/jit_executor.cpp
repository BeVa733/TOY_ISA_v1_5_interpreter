#include "execution_type/jit_executor.hpp"
#include "execution_type/threaded_executor.hpp"
#include "toy/basic_block_cache.hpp"
#include "toy/execution_context.hpp"
#include "toy/executor.hpp"
#include "toy/toy_constants.hpp"

#include <ir.h>
#include <ir_builder.h>

#include <cstddef>
#include <stdexcept>
#include <utility>

namespace {

using TIInstructionHandler = void (*)(ExecutionContext &,
                                      const TIInstruction &);

template <typename TAction>
void executeHelper(ExecutionContext *Context, TAction Action) noexcept {
  try {
    Action();
  } catch (...) {
    Context->state().fault();
  }
}

void executeInstructionHelper(ExecutionContext *Context,
                              const TIInstruction *Instruction,
                              TIInstructionHandler Handler) noexcept {
  executeHelper(Context, [&] { Handler(*Context, *Instruction); });
}

void encodeExitIfStopped(ir_ctx *ctx, ExecutionContext &Context) {
  ir_ref Status  = ir_LOAD_U8(ir_CONST_ADDR(&Context.state().Status));
  ir_ref Stopped = ir_IF(ir_NE(
      Status, ir_CONST_U8(static_cast<uint8_t>(ExecutionStatus::RUNNING))));
  ir_IF_TRUE(Stopped);
  ir_RETURN(ir_CONST_ADDR(0));
  ir_IF_FALSE(Stopped);
}

void encodeCall(ir_ctx *ctx, ir_ref ContextReference, ExecutionContext &Context,
                const TIInstruction &Instruction,
                TIInstructionHandler Handler) {
  ir_ref HandlerReference     = ir_CONST_ADDR(Handler);
  ir_ref InstructionReference = ir_CONST_ADDR(&Instruction);
  ir_CALL_3(IR_VOID, ir_CONST_ADDR(executeInstructionHelper), ContextReference,
            InstructionReference, HandlerReference);
  encodeExitIfStopped(ctx, Context);
}

void encodeLi(ir_ctx *ctx, ExecutionContext &Context,
              const TIInstruction &Inst) {
  uint32_t *RegPtr = &(Context.cpu().Registers[Inst.Operands[0].Value]);
  uint32_t Value   = Inst.Operands[1].Value;
  ir_STORE(ir_CONST_ADDR(RegPtr), ir_CONST_U32(Value));
}

void encodeAdd(ir_ctx *ctx, ExecutionContext &Context,
               const TIInstruction &Inst) {
  uint32_t *RdPtr = &Context.cpu().Registers[Inst.Operands[0].Value];
  uint32_t *RsPtr = &Context.cpu().Registers[Inst.Operands[1].Value];
  uint32_t *RtPtr = &Context.cpu().Registers[Inst.Operands[2].Value];

  ir_ref RsVal    = ir_LOAD_U32(ir_CONST_ADDR(RsPtr));
  ir_ref RtVal    = ir_LOAD_U32(ir_CONST_ADDR(RtPtr));
  ir_ref NewRdVal = ir_ADD_U32(RsVal, RtVal);

  ir_STORE(ir_CONST_ADDR(RdPtr), NewRdVal);
}

void encodeAddi(ir_ctx *ctx, ExecutionContext &Context,
                const TIInstruction &Inst) {
  uint32_t *RdPtr = &Context.cpu().Registers[Inst.Operands[0].Value];
  uint32_t *RsPtr = &Context.cpu().Registers[Inst.Operands[1].Value];
  uint32_t Imm    = Inst.Operands[2].Value;

  ir_ref RsVal    = ir_LOAD_U32(ir_CONST_ADDR(RsPtr));
  ir_ref NewRdVal = ir_ADD_U32(RsVal, ir_CONST_U32(Imm));

  ir_STORE(ir_CONST_ADDR(RdPtr), NewRdVal);
}

void encodeOr(ir_ctx *ctx, ExecutionContext &Context,
              const TIInstruction &Inst) {
  uint32_t *RdPtr = &Context.cpu().Registers[Inst.Operands[0].Value];
  uint32_t *RsPtr = &Context.cpu().Registers[Inst.Operands[1].Value];
  uint32_t *RtPtr = &Context.cpu().Registers[Inst.Operands[2].Value];

  ir_ref RsVal    = ir_LOAD_U32(ir_CONST_ADDR(RsPtr));
  ir_ref RtVal    = ir_LOAD_U32(ir_CONST_ADDR(RtPtr));
  ir_ref NewRdVal = ir_OR_U32(RsVal, RtVal);
  ir_STORE(ir_CONST_ADDR(RdPtr), NewRdVal);
}

void encodeClz(ir_ctx *ctx, ExecutionContext &Context,
               const TIInstruction &Inst) {
  uint32_t *RdPtr = &Context.cpu().Registers[Inst.Operands[0].Value];
  uint32_t *RsPtr = &Context.cpu().Registers[Inst.Operands[1].Value];

  ir_ref RsVal = ir_LOAD_U32(ir_CONST_ADDR(RsPtr));

  ir_ref IsZero         = ir_EQ(RsVal, ir_CONST_U32(0));
  ir_ref ZeroAdjustment = ir_ZEXT_U32(IsZero);
  ir_ref SafeValue      = ir_OR_U32(RsVal, ZeroAdjustment);
  ir_ref Value          = ir_ADD_U32(ir_CTLZ_U32(SafeValue), ZeroAdjustment);

  ir_STORE(ir_CONST_ADDR(RdPtr), Value);
}

void encodeSsat(ir_ctx *ctx, ir_ref ContextReference, ExecutionContext &Context,
                const TIInstruction &Inst) {
  uint32_t *RdPtr = &Context.cpu().Registers[Inst.Operands[0].Value];
  uint32_t *RsPtr = &Context.cpu().Registers[Inst.Operands[1].Value];
  uint32_t Width  = Inst.Operands[2].Value;

  // for raise exception
  if (Width == 0 || Width >= TI32::WORD_BIT_COUNT) {
    return encodeCall(ctx, ContextReference, Context, Inst, executeSsat);
  }

  int32_t Maximum = static_cast<int32_t>((uint32_t{1} << (Width - 1)) - 1);
  int32_t Minimum = -Maximum - 1;

  ir_ref RsVal       = ir_LOAD_U32(ir_CONST_ADDR(RsPtr));
  ir_ref SignedRsVal = ir_BITCAST_I32(RsVal);
  ir_ref LowerBound  = ir_MAX_I32(SignedRsVal, ir_CONST_I32(Minimum));
  ir_ref Saturated   = ir_MIN_I32(LowerBound, ir_CONST_I32(Maximum));

  ir_STORE(ir_CONST_ADDR(RdPtr), ir_BITCAST_U32(Saturated));
}

void encodeRori(ir_ctx *ctx, ExecutionContext &Context,
                const TIInstruction &Inst) {
  uint32_t *RdPtr = &Context.cpu().Registers[Inst.Operands[0].Value];
  uint32_t *RsPtr = &Context.cpu().Registers[Inst.Operands[1].Value];
  uint32_t Shift  = Inst.Operands[2].Value;

  ir_ref RsVal = ir_LOAD_U32(ir_CONST_ADDR(RsPtr));
  ir_ref Value = ir_ROR_U32(RsVal, ir_CONST_U32(Shift));

  ir_STORE(ir_CONST_ADDR(RdPtr), Value);
}

void encodeBext(ir_ctx *ctx, ExecutionContext &Context,
                const TIInstruction &Inst) {
  uint32_t *RdPtr  = &Context.cpu().Registers[Inst.Operands[0].Value];
  uint32_t *Rs1Ptr = &Context.cpu().Registers[Inst.Operands[1].Value];
  uint32_t *Rs2Ptr = &Context.cpu().Registers[Inst.Operands[2].Value];

  ir_ref Rs1Val = ir_LOAD_U32(ir_CONST_ADDR(Rs1Ptr));
  ir_ref Rs2Val = ir_LOAD_U32(ir_CONST_ADDR(Rs2Ptr));
  ir_ref Value  = ir_CALL_2(IR_U32, ir_CONST_ADDR(ExecutorDetail::compressBits),
                            Rs1Val, Rs2Val);

  ir_STORE(ir_CONST_ADDR(RdPtr), Value);
}

// ---------- Memory work helpers ----------------------------------
uint32_t readMemory32(ExecutionContext *Context, uint32_t Address) noexcept {
  uint32_t Value = 0;
  executeHelper(Context, [&] { Value = Context->memory().read32(Address); });
  return Value;
}

void writeMemory32(ExecutionContext *Context, uint32_t Address,
                   uint32_t Value) noexcept {
  executeHelper(Context, [&] { Context->memory().write32(Address, Value); });
}
// -----------------------------------------------------------------

void encodeLdImm(ir_ctx *ctx, ir_ref ContextReference,
                 ExecutionContext &Context, const TIInstruction &Inst) {
  uint32_t *RtPtr   = &Context.cpu().Registers[Inst.Operands[0].Value];
  uint32_t *BasePtr = &Context.cpu().Registers[Inst.Operands[1].Value];
  uint32_t Imm      = Inst.Operands[2].Value;

  ir_ref BaseVal = ir_LOAD_U32(ir_CONST_ADDR(BasePtr));
  ir_ref Addr    = ir_ADD_U32(BaseVal, ir_CONST_U32(Imm));
  ir_ref Value =
      ir_CALL_2(IR_U32, ir_CONST_ADDR(readMemory32), ContextReference, Addr);

  encodeExitIfStopped(ctx, Context);
  ir_STORE(ir_CONST_ADDR(RtPtr), Value);
}

void encodeLdReg(ir_ctx *ctx, ir_ref ContextReference,
                 ExecutionContext &Context, const TIInstruction &Inst) {
  uint32_t *RtPtr   = &Context.cpu().Registers[Inst.Operands[0].Value];
  uint32_t *BasePtr = &Context.cpu().Registers[Inst.Operands[1].Value];
  uint32_t *RmPtr   = &Context.cpu().Registers[Inst.Operands[2].Value];

  ir_ref BaseVal = ir_LOAD_U32(ir_CONST_ADDR(BasePtr));
  ir_ref RmVal   = ir_LOAD_U32(ir_CONST_ADDR(RmPtr));
  ir_ref Addr    = ir_ADD_U32(BaseVal, RmVal);
  ir_ref Value =
      ir_CALL_2(IR_U32, ir_CONST_ADDR(readMemory32), ContextReference, Addr);

  encodeExitIfStopped(ctx, Context);
  ir_STORE(ir_CONST_ADDR(RtPtr), Value);
}

void encodeSt(ir_ctx *ctx, ir_ref ContextReference, ExecutionContext &Context,
              const TIInstruction &Inst) {
  uint32_t *RtPtr   = &Context.cpu().Registers[Inst.Operands[0].Value];
  uint32_t *BasePtr = &Context.cpu().Registers[Inst.Operands[1].Value];
  uint32_t Imm      = Inst.Operands[2].Value;

  ir_ref BaseVal = ir_LOAD_U32(ir_CONST_ADDR(BasePtr));
  ir_ref Addr    = ir_ADD_U32(BaseVal, ir_CONST_U32(Imm));
  ir_ref Value   = ir_LOAD_U32(ir_CONST_ADDR(RtPtr));

  ir_CALL_3(IR_VOID, ir_CONST_ADDR(writeMemory32), ContextReference, Addr,
            Value);
  encodeExitIfStopped(ctx, Context);
}

void encodeStp(ir_ctx *ctx, ir_ref ContextReference, ExecutionContext &Context,
               const TIInstruction &Inst) {
  uint32_t *Rt1Ptr  = &Context.cpu().Registers[Inst.Operands[0].Value];
  uint32_t *Rt2Ptr  = &Context.cpu().Registers[Inst.Operands[1].Value];
  uint32_t *BasePtr = &Context.cpu().Registers[Inst.Operands[2].Value];
  uint32_t Imm      = Inst.Operands[3].Value;

  ir_ref BaseVal = ir_LOAD_U32(ir_CONST_ADDR(BasePtr));
  ir_ref Addr1   = ir_ADD_U32(BaseVal, ir_CONST_U32(Imm));
  ir_ref Addr2   = ir_ADD_U32(Addr1, ir_CONST_U32(TI32::INSTRUCTION_SIZE));

  ir_ref Value1 = ir_LOAD_U32(ir_CONST_ADDR(Rt1Ptr));
  ir_ref Value2 = ir_LOAD_U32(ir_CONST_ADDR(Rt2Ptr));

  ir_CALL_3(IR_VOID, ir_CONST_ADDR(writeMemory32), ContextReference, Addr1,
            Value1);
  encodeExitIfStopped(ctx, Context);
  ir_CALL_3(IR_VOID, ir_CONST_ADDR(writeMemory32), ContextReference, Addr2,
            Value2);
  encodeExitIfStopped(ctx, Context);
}

// Branch encoders

void encodePc(ir_ctx *ctx, ExecutionContext &Context, uint32_t Value) {
  ir_STORE(ir_CONST_ADDR(&Context.cpu().PC), ir_CONST_U32(Value));
}

void encodeJ(ir_ctx *ctx, ExecutionContext &Context, const TIInstruction &Inst,
             uint32_t InstructionAddress) {
  uint32_t Target = (InstructionAddress & 0xF0000000) |
                    (Inst.Operands[0].Value * TI32::INSTRUCTION_SIZE);
  encodePc(ctx, Context, Target);
}

void encodeInstruction(ir_ctx *ctx, ExecutionContext &Context,
                       ir_ref ContextReference,
                       const TIInstruction &Instruction,
                       uint32_t InstructionAddress) {
  switch (Instruction.OpCode) {
  case TIOpcode::LI:
    return encodeLi(ctx, Context, Instruction);
  case TIOpcode::ADD:
    return encodeAdd(ctx, Context, Instruction);
  case TIOpcode::ADDI:
    return encodeAddi(ctx, Context, Instruction);
  case TIOpcode::OR:
    return encodeOr(ctx, Context, Instruction);
  case TIOpcode::LDreg:
    return encodeLdReg(ctx, ContextReference, Context, Instruction);
  case TIOpcode::ST:
    return encodeSt(ctx, ContextReference, Context, Instruction);
  case TIOpcode::STP:
    return encodeStp(ctx, ContextReference, Context, Instruction);
  case TIOpcode::FLUSH:
    encodeCall(ctx, ContextReference, Context, Instruction, executeFlush);
    return encodePc(ctx, Context, InstructionAddress + TI32::INSTRUCTION_SIZE);
  case TIOpcode::J:
    return encodeJ(ctx, Context, Instruction, InstructionAddress);
  case TIOpcode::CLZ:
    return encodeClz(ctx, Context, Instruction);
  case TIOpcode::SSAT:
    return encodeSsat(ctx, ContextReference, Context, Instruction);
  case TIOpcode::RORI:
    return encodeRori(ctx, Context, Instruction);
  case TIOpcode::BEXT:
    return encodeBext(ctx, Context, Instruction);
  case TIOpcode::SYSCALL:
    encodePc(ctx, Context, InstructionAddress);
    return encodeCall(ctx, ContextReference, Context, Instruction,
                      executeSyscall);
  case TIOpcode::LDimm:
    return encodeLdImm(ctx, ContextReference, Context, Instruction);
  case TIOpcode::INVALID:
  default:
    encodePc(ctx, Context, InstructionAddress);
    return encodeCall(ctx, ContextReference, Context, Instruction,
                      executeInvalid);
  }
}

bool updatesPc(TIOpcode OpCode) {
  switch (OpCode) {
  case TIOpcode::BEQ:
  case TIOpcode::J:
  case TIOpcode::SYSCALL:
  case TIOpcode::INVALID:
  case TIOpcode::FLUSH:
    return true;

  default:
    return false;
  }
}

ir_ref encodeJitEntryLoad(ir_ctx *ctx, TIBasicBlockCache &Cache,
                          uint32_t Address) {
  return ir_LOAD_A(ir_CONST_ADDR(Cache.jitEntrySlot(Address)));
}

ir_ref encodeNextJitEntry(ir_ctx *ctx, ExecutionContext &Context,
                          TIBasicBlockCache &Cache, const TIBasicBlock &Block) {
  const auto &Instructions = Block.instructions();
  if (Instructions.empty()) {
    return encodeJitEntryLoad(ctx, Cache, Block.endAddress());
  }

  const TIInstruction &LastInstruction = Instructions.back();
  uint32_t InstructionAddress = Block.endAddress() - TI32::INSTRUCTION_SIZE;

  switch (LastInstruction.OpCode) {
  case TIOpcode::BEQ: {
    uint32_t *RsPtr =
        &Context.cpu().Registers[LastInstruction.Operands[0].Value];
    uint32_t *RtPtr =
        &Context.cpu().Registers[LastInstruction.Operands[1].Value];
    uint32_t TakenAddress =
        InstructionAddress +
        LastInstruction.Operands[2].Value * TI32::INSTRUCTION_SIZE;
    uint32_t FallthroughAddress = InstructionAddress + TI32::INSTRUCTION_SIZE;

    ir_ref RsVal = ir_LOAD_U32(ir_CONST_ADDR(RsPtr));
    ir_ref RtVal = ir_LOAD_U32(ir_CONST_ADDR(RtPtr));

    ir_ref Condition = ir_IF(ir_EQ(RsVal, RtVal));
    ir_IF_TRUE(Condition);
    ir_ref TakenEntry = encodeJitEntryLoad(ctx, Cache, TakenAddress);
    ir_STORE(ir_CONST_ADDR(&Context.cpu().PC), ir_CONST_U32(TakenAddress));
    ir_ref TakenEnd = ir_END();

    ir_IF_FALSE(Condition);
    ir_ref FallthroughEntry =
        encodeJitEntryLoad(ctx, Cache, FallthroughAddress);
    ir_STORE(ir_CONST_ADDR(&Context.cpu().PC),
             ir_CONST_U32(FallthroughAddress));
    ir_ref FallthroughEnd = ir_END();

    ir_MERGE_2(TakenEnd, FallthroughEnd);
    return ir_PHI_2(IR_ADDR, TakenEntry, FallthroughEntry);
  }
  case TIOpcode::J: {
    uint32_t Target =
        (InstructionAddress & 0xF0000000) |
        (LastInstruction.Operands[0].Value * TI32::INSTRUCTION_SIZE);
    return encodeJitEntryLoad(ctx, Cache, Target);
  }
  case TIOpcode::INVALID:
    return ir_CONST_ADDR(0);
  case TIOpcode::SYSCALL:
    return encodeJitEntryLoad(ctx, Cache,
                              InstructionAddress + TI32::INSTRUCTION_SIZE);
  case TIOpcode::FLUSH:
    return ir_CONST_ADDR(0);
  default:
    return encodeJitEntryLoad(ctx, Cache, Block.endAddress());
  }
}

TICompiledBlock compileBlock(ir_ctx *ctx, ExecutionContext &Context,
                             TIBasicBlockCache &Cache,
                             const TIBasicBlock &Block,
                             uint8_t OptimizationLevel) {
  ir_START();
  ctx->ret_type           = IR_ADDR;
  ir_ref ContextReference = ir_PARAM(IR_ADDR, "context", 1);

  const auto &Instructions = Block.instructions();
  for (std::size_t Index = 0; Index < Block.instructionCount(); ++Index) {
    const bool IsTerminalBeq = Index + 1 == Block.instructionCount() &&
                               Instructions[Index].OpCode == TIOpcode::BEQ;
    if (IsTerminalBeq) {
      continue;
    }

    uint32_t InstructionAddress =
        Block.startAddress() +
        static_cast<uint32_t>(Index * TI32::INSTRUCTION_SIZE);
    encodeInstruction(ctx, Context, ContextReference, Instructions[Index],
                      InstructionAddress);
  }

  if (Instructions.empty() || !updatesPc(Instructions.back().OpCode)) {
    encodePc(ctx, Context, Block.endAddress());
  }

  ir_RETURN(encodeNextJitEntry(ctx, Context, Cache, Block));
  std::size_t CodeSize     = 0;
  TIJitFunction EntryPoint = reinterpret_cast<TIJitFunction>(
      ir_jit_compile(ctx, OptimizationLevel, &CodeSize));
  return TICompiledBlock(EntryPoint, CodeSize);
}

} // namespace

void TICompiledBlock::reset() {
  if (EntryPoint_ != nullptr) {
    ir_mem_unmap(reinterpret_cast<void *>(EntryPoint_), CodeSize_);
    EntryPoint_ = nullptr;
    CodeSize_   = 0;
  }
}

void executeJitBlock(ExecutionContext &Context, TIBasicBlockCache &Cache,
                     TIBasicBlock &Block, uint64_t Threshold) {
  if (!Block.hasCompiledBlock() && Block.executionCount() < Threshold) {
    Block.recordExecution();
    executeThreadedBlock(Context, Block);
    return;
  }

  if (!Block.hasCompiledBlock()) {
    ir_ctx IrContext{};
    const uint32_t IrFlags =
        IR_FUNCTION | (Context.jitOptimizationLevel() > 0 ? IR_OPT_FOLDING : 0);
    ir_init(&IrContext, IrFlags, IR_CONSTS_LIMIT_MIN, IR_INSNS_LIMIT_MIN);

    TICompiledBlock CompiledBlock = compileBlock(
        &IrContext, Context, Cache, Block, Context.jitOptimizationLevel());
    ir_free(&IrContext);

    if (CompiledBlock.empty()) {
      throw std::runtime_error("IR failed to compile a basic block");
    }

    Block.setCompiledBlock(std::move(CompiledBlock));
    Cache.setJitEntry(Block.startAddress(), Block.compiledBlock().entryPoint());
  }

  TIJitFunction NextEntry = Block.compiledBlock().execute(Context);
  while (NextEntry != nullptr) {
    NextEntry = reinterpret_cast<TIJitFunction>(NextEntry(&Context));
  }
}
