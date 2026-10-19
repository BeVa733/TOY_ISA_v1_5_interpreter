#include "../../include/execution_type/jit_executor.hpp"
#include "../../include/toy/basic_block_cache.hpp"
#include "../../include/toy/execution_context.hpp"
#include "../../include/toy/executor.hpp"
#include "../../include/toy/toy_constants.hpp"

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

// uses until support inc PC once after block
#define INCREMENT_PC                                                           \
  uint32_t *PCPtr  = &(Context.cpu().PC);                                      \
  ir_ref PCAddr    = ir_CONST_ADDR(PCPtr);                                     \
  ir_ref CurrentPC = ir_LOAD_U32(PCAddr);                                      \
  ir_ref NewPC     = ir_ADD_U32(CurrentPC, ir_CONST_U32(4));                   \
  ir_STORE(PCAddr, NewPC);

void encodeLi(ir_ctx *ctx, ExecutionContext &Context,
              const TIInstruction &Inst) {
  uint32_t *RegPtr = &(Context.cpu().Registers[Inst.Operands[0].Value]);
  uint32_t Value   = Inst.Operands[1].Value;
  ir_STORE(ir_CONST_ADDR(RegPtr), ir_CONST_U32(Value));

  INCREMENT_PC
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

  INCREMENT_PC
}

void encodeAddi(ir_ctx *ctx, ExecutionContext &Context,
                const TIInstruction &Inst) {
  uint32_t *RdPtr = &Context.cpu().Registers[Inst.Operands[0].Value];
  uint32_t *RsPtr = &Context.cpu().Registers[Inst.Operands[1].Value];
  uint32_t Imm    = Inst.Operands[2].Value;

  ir_ref RsVal    = ir_LOAD_U32(ir_CONST_ADDR(RsPtr));
  ir_ref NewRdVal = ir_ADD_U32(RsVal, ir_CONST_U32(Imm));

  ir_STORE(ir_CONST_ADDR(RdPtr), NewRdVal);

  INCREMENT_PC
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

  INCREMENT_PC
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

  INCREMENT_PC
}

void encodeSsat(ir_ctx *ctx, ir_ref ContextReference, ExecutionContext &Context,
                const TIInstruction &Inst) {
  uint32_t *RdPtr = &Context.cpu().Registers[Inst.Operands[0].Value];
  uint32_t *RsPtr = &Context.cpu().Registers[Inst.Operands[1].Value];
  uint32_t Width  = Inst.Operands[2].Value;

	// for raise exception
  if (Width == 0 || Width >= TI32::WORD_BIT_COUNT) {
    return encodeCall(ctx, ContextReference, Inst, executeSsat);
  }

  int32_t Maximum = static_cast<int32_t>((uint32_t{1} << (Width - 1)) - 1);
  int32_t Minimum = -Maximum - 1;

  ir_ref RsVal       = ir_LOAD_U32(ir_CONST_ADDR(RsPtr));
  ir_ref SignedRsVal = ir_BITCAST_I32(RsVal);
  ir_ref LowerBound  = ir_MAX_I32(SignedRsVal, ir_CONST_I32(Minimum));
  ir_ref Saturated   = ir_MIN_I32(LowerBound, ir_CONST_I32(Maximum));

  ir_STORE(ir_CONST_ADDR(RdPtr), ir_BITCAST_U32(Saturated));

  INCREMENT_PC
}

void encodeRori(ir_ctx *ctx, ExecutionContext &Context,
                const TIInstruction &Inst) {
  uint32_t *RdPtr = &Context.cpu().Registers[Inst.Operands[0].Value];
  uint32_t *RsPtr = &Context.cpu().Registers[Inst.Operands[1].Value];
  uint32_t Shift  = Inst.Operands[2].Value;

  ir_ref RsVal = ir_LOAD_U32(ir_CONST_ADDR(RsPtr));
  ir_ref Value = ir_ROR_U32(RsVal, ir_CONST_U32(Shift));

  ir_STORE(ir_CONST_ADDR(RdPtr), Value);

  INCREMENT_PC
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

  INCREMENT_PC
}

// ---------- Memory work helpers ----------------------------------
uint32_t readMemory32(ExecutionContext *Context, uint32_t Address) {
  return Context->memory().read32(Address);
}

void writeMemory32(ExecutionContext *Context, uint32_t Address,
                   uint32_t Value) {
  Context->memory().write32(Address, Value);
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

  ir_STORE(ir_CONST_ADDR(RtPtr), Value);

  INCREMENT_PC
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

  ir_STORE(ir_CONST_ADDR(RtPtr), Value);

  INCREMENT_PC
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

  INCREMENT_PC
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
  ir_CALL_3(IR_VOID, ir_CONST_ADDR(writeMemory32), ContextReference, Addr2,
            Value2);

  INCREMENT_PC
}

// Branch encoders

void encodeJ(ir_ctx *ctx, ExecutionContext &Context,
             const TIInstruction &Inst) {
  uint32_t *PCPtr = &Context.cpu().PC;
  uint32_t Target = Inst.Operands[0].Value * TI32::INSTRUCTION_SIZE;

  ir_ref PCAddr = ir_CONST_ADDR(PCPtr);
  ir_ref PCVal  = ir_LOAD_U32(PCAddr);
  ir_ref Page   = ir_AND_U32(PCVal, ir_CONST_U32(0xF0000000));
  ir_ref NewPC  = ir_OR_U32(Page, ir_CONST_U32(Target));

  ir_STORE(PCAddr, NewPC);
}

void encodeBeq(ir_ctx *ctx, ExecutionContext &Context,
               const TIInstruction &Inst) {
  uint32_t *RsPtr = &Context.cpu().Registers[Inst.Operands[0].Value];
  uint32_t *RtPtr = &Context.cpu().Registers[Inst.Operands[1].Value];
  uint32_t Offset = Inst.Operands[2].Value;
  uint32_t *PCPtr = &(Context.cpu().PC);

  ir_ref RsVal  = ir_LOAD_U32(ir_CONST_ADDR(RsPtr));
  ir_ref RtVal  = ir_LOAD_U32(ir_CONST_ADDR(RtPtr));
  ir_ref PCAddr = ir_CONST_ADDR(PCPtr);
  ir_ref PCVal  = ir_LOAD_U32(PCAddr);

  ir_ref cond = ir_IF(ir_EQ(RsVal, RtVal));
  ir_IF_TRUE(cond);
  ir_ref NewPC =
      ir_ADD_U32(PCVal, ir_CONST_U32(Offset * TI32::INSTRUCTION_SIZE));
  ir_ref end_true = ir_END();
  ir_IF_FALSE(cond);
  ir_ref IncPC     = ir_ADD_U32(PCVal, ir_CONST_U32(TI32::INSTRUCTION_SIZE));
  ir_ref end_false = ir_END();

  ir_MERGE_2(end_true, end_false);
  ir_ref ResultPC = ir_PHI_2(IR_U32, NewPC, IncPC);
  ir_STORE(PCAddr, ResultPC);
}

#undef INCREMENT_PC

void encodeInstruction(ir_ctx *ctx, ExecutionContext &Context,
                       ir_ref ContextReference,
                       const TIInstruction &Instruction) {
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
  case TIOpcode::BEQ:
    return encodeBeq(ctx, Context, Instruction);
  case TIOpcode::J:
    return encodeJ(ctx, Context, Instruction);
  case TIOpcode::CLZ:
    return encodeClz(ctx, Context, Instruction);
  case TIOpcode::SSAT:
    return encodeSsat(ctx, ContextReference, Context, Instruction);
  case TIOpcode::RORI:
    return encodeRori(ctx, Context, Instruction);
  case TIOpcode::BEXT:
    return encodeBext(ctx, Context, Instruction);
  case TIOpcode::SYSCALL:
    return encodeCall(ctx, ContextReference, Instruction, executeSyscall);
  case TIOpcode::LDimm:
    return encodeLdImm(ctx, ContextReference, Context, Instruction);
  case TIOpcode::INVALID:
  default:
    return encodeCall(ctx, ContextReference, Instruction, executeInvalid);
  }
}

TIJitBlock encodeBlock(ir_ctx *ctx, ExecutionContext &Context,
                       const TIBasicBlock &Block, std::size_t &CodeSize) {
  ir_START();
  ir_ref ContextReference = ir_PARAM(IR_ADDR, "context", 1);

  for (const TIInstruction &Instruction : Block.instructions()) {
    encodeInstruction(ctx, Context, ContextReference, Instruction);
  }

  ir_RETURN(IR_UNUSED);
  return reinterpret_cast<TIJitBlock>(ir_jit_compile(ctx, 0, &CodeSize));
}

} // namespace

void executeJitBlock(ExecutionContext &Context, const TIBasicBlock &Block) {
  ir_ctx IrContext{};
  ir_init(&IrContext, IR_FUNCTION, IR_CONSTS_LIMIT_MIN, IR_INSNS_LIMIT_MIN);

  std::size_t CodeSize = 0;
  TIJitBlock JitBlock  = encodeBlock(&IrContext, Context, Block, CodeSize);
  if (JitBlock == nullptr) {
    ir_free(&IrContext);
    throw std::runtime_error("IR failed to compile a basic block");
  }

  JitBlock(&Context);

  ir_mem_unmap(reinterpret_cast<void *>(JitBlock), CodeSize);
  ir_free(&IrContext);
}
