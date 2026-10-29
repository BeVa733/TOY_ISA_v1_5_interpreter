#include "execution_type/threaded_executor.hpp"
#include "toy/basic_block_cache.hpp"
#include "toy/execution_context.hpp"
#include "toy/executor.hpp"

#if defined(__clang__)
#if __has_cpp_attribute(clang::musttail)
#define TI_MUSTTAIL [[clang::musttail]]
#else
#error "Threaded execution requires Clang with musttail support"
#endif

#elif defined(__GNUC__)
#if __has_cpp_attribute(gnu::musttail)
#define TI_MUSTTAIL [[gnu::musttail]]
#else
#error "Threaded execution requires GCC with musttail support"
#endif

#else
#error "Threaded execution requires a compiler with musttail support"
#endif

namespace {

#define DISPATCH_NEXT(Context, Current, End)                                   \
  ++Current;                                                                   \
  if (Current == End) {                                                        \
    return;                                                                    \
  }                                                                            \
  TI_MUSTTAIL return Current->ThreadedHandler(Context, Current, End)

#define DEFINE_THREADED_HANDLER(Name)                                          \
  void threaded##Name(ExecutionContext &Context, const TIInstruction *Current, \
                      const TIInstruction *End) {                              \
    execute##Name(Context, *Current);                                          \
    DISPATCH_NEXT(Context, Current, End);                                      \
  }

DEFINE_THREADED_HANDLER(Li)
DEFINE_THREADED_HANDLER(Add)
DEFINE_THREADED_HANDLER(Addi)
DEFINE_THREADED_HANDLER(Or)
DEFINE_THREADED_HANDLER(LdReg)
DEFINE_THREADED_HANDLER(LdImm)
DEFINE_THREADED_HANDLER(St)
DEFINE_THREADED_HANDLER(Stp)
DEFINE_THREADED_HANDLER(Clz)
DEFINE_THREADED_HANDLER(Ssat)
DEFINE_THREADED_HANDLER(Rori)
DEFINE_THREADED_HANDLER(Bext)

void threadedStSmc(ExecutionContext &Context, const TIInstruction *Current,
                   const TIInstruction *) {
  executeStSmc(Context, *Current);
}

void threadedStpSmc(ExecutionContext &Context, const TIInstruction *Current,
                    const TIInstruction *) {
  executeStpSmc(Context, *Current);
}

void threadedBeq(ExecutionContext &Context, const TIInstruction *Current,
                 const TIInstruction *) {
  executeBeq(Context, *Current);
}
void threadedJ(ExecutionContext &Context, const TIInstruction *Current,
               const TIInstruction *) {
  executeJ(Context, *Current);
}
void threadedSyscall(ExecutionContext &Context, const TIInstruction *Current,
                     const TIInstruction *) {
  executeSyscall(Context, *Current);
}
void threadedInvalid(ExecutionContext &Context, const TIInstruction *Current,
                     const TIInstruction *) {
  executeInvalid(Context, *Current);
}

} // namespace

TIThreadedHandler getThreadedHandler(TIOpcode OpCode) {
  switch (OpCode) {
  case TIOpcode::LI:
    return threadedLi;
  case TIOpcode::ADD:
    return threadedAdd;
  case TIOpcode::ADDI:
    return threadedAddi;
  case TIOpcode::OR:
    return threadedOr;
  case TIOpcode::LDreg:
    return threadedLdReg;
  case TIOpcode::ST:
    return threadedSt;
  case TIOpcode::STP:
    return threadedStp;
  case TIOpcode::ST_SMC:
    return threadedStSmc;
  case TIOpcode::STP_SMC:
    return threadedStpSmc;
  case TIOpcode::BEQ:
    return threadedBeq;
  case TIOpcode::J:
    return threadedJ;
  case TIOpcode::CLZ:
    return threadedClz;
  case TIOpcode::SSAT:
    return threadedSsat;
  case TIOpcode::RORI:
    return threadedRori;
  case TIOpcode::BEXT:
    return threadedBext;
  case TIOpcode::SYSCALL:
    return threadedSyscall;
  case TIOpcode::LDimm:
    return threadedLdImm;
  case TIOpcode::INVALID:
  default:
    return threadedInvalid;
  }
}

void executeThreadedBlock(ExecutionContext &Context,
                          const TIBasicBlock &Block) {
  const auto &Instructions     = Block.instructions();
  const TIInstruction *Current = Instructions.data();
  const TIInstruction *End     = Current + Instructions.size();
  Current->ThreadedHandler(Context, Current, End);
}

#undef DEFINE_THREADED_HANDLER
#undef DISPATCH_NEXT
#undef TI_MUSTTAIL
