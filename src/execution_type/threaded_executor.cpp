#include "../../include/execution_type/threaded_executor.hpp"
#include "../../include/toy/basic_block_cache.hpp"
#include "../../include/toy/execution_context.hpp"
#include "../../include/toy/executor.hpp"

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

#define DISPATCH_NEXT(Thread)                                                  \
  ++(Thread).Current;                                                          \
  if ((Thread).Current == (Thread).End) {                                      \
    return;                                                                    \
  }                                                                            \
  TI_MUSTTAIL return (Thread).Current->ThreadedHandler(Thread)

#define DEFINE_THREADED_HANDLER(Name)                                          \
  void threaded##Name(TIThreadState &Thread) {                                 \
    execute##Name(Thread.Context, *Thread.Current);                            \
    DISPATCH_NEXT(Thread);                                                     \
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

void threadedBeq(TIThreadState &Thread) {
  executeBeq(Thread.Context, *Thread.Current);
}
void threadedJ(TIThreadState &Thread) {
  executeJ(Thread.Context, *Thread.Current);
}
void threadedSyscall(TIThreadState &Thread) {
  executeSyscall(Thread.Context, *Thread.Current);
}
void threadedInvalid(TIThreadState &Thread) {
  executeInvalid(Thread.Context, *Thread.Current);
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
  const auto &Instructions = Block.instructions();
  TIThreadState Thread(Context, Instructions.data(),
                       Instructions.data() + Instructions.size());
  Thread.Current->ThreadedHandler(Thread);
}

#undef DEFINE_THREADED_HANDLER
#undef DISPATCH_NEXT
#undef TI_MUSTTAIL
