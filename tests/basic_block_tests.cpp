#include "../include/execution_type/jit_executor.hpp"
#include "../include/toy/basic_block_cache.hpp"
#include "../include/toy/execution_context.hpp"
#include "test_utils.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void testCache(const std::string &Filename) {
  ExecutionContext Context(48);
  Context.loadBinary(Filename);

  TIMemory &Memory = Context.memory();
  TIDecoder Decoder{};
  TIBasicBlockCache Cache{};

  TIBasicBlock &Jump = Cache.getBlock(0, Memory, Decoder);
  require(Jump.startAddress() == 0, "J block has incorrect start address");
  require(Jump.instructions().size() == 3, "J did not end its block");
  require(Jump.instructions().back().OpCode == TIOpcode::J,
          "J block has incorrect last instruction");
  require(&Cache.getBlock(0, Memory, Decoder) == &Jump,
          "Cached block was decoded again");

  executeJitBlock(Context, Jump, 1);
  require(Jump.executionCount() == 1 && !Jump.hasCompiledBlock(),
          "JIT compiled a block before its threshold");
  Context.cpu().PC = 0;
  executeJitBlock(Context, Jump, 1);
  require(Jump.hasCompiledBlock(),
          "JIT did not cache a block after its threshold");

  TIBasicBlock &Overlap = Cache.getBlock(4, Memory, Decoder);
  require(Overlap.instructions().size() == 2,
          "Overlapping block has incorrect size");
  require(&Overlap != &Jump, "Overlapping block was not created");

  const TIBasicBlock &Branch = Cache.getBlock(16, Memory, Decoder);
  require(Branch.instructions().size() == 2, "BEQ did not end its block");
  require(Branch.instructions().back().OpCode == TIOpcode::BEQ,
          "BEQ block has incorrect last instruction");

  const TIBasicBlock &Syscall = Cache.getBlock(28, Memory, Decoder);
  require(Syscall.instructions().size() == 2, "SYSCALL did not end its block");
  require(Syscall.instructions().back().OpCode == TIOpcode::SYSCALL,
          "SYSCALL block has incorrect last instruction");

  const TIBasicBlock *BranchAddress = &Branch;
  Memory.setWriteObserver([&Cache](uint32_t Address, std::size_t ByteCount) {
    Cache.invalidateRange(Address, ByteCount);
  });
  Memory.write32(0, 0);
  const TIBasicBlock &Invalidated = Cache.getBlock(0, Memory, Decoder);
  require(Invalidated.instructions().front().OpCode == TIOpcode::INVALID,
          "Code write did not invalidate the affected block");
  require(&Cache.getBlock(16, Memory, Decoder) == BranchAddress,
          "Code write invalidated an unaffected block");
}

void testExecution(const std::string &Filename, TIExecutionMode Mode) {
  ExecutionContext Context(64, Mode);
  Context.loadBinary(Filename);

  Context.step();

  require(Context.cpu().Registers[2] == 12,
          "step did not execute the complete block");
  require(Context.state().Status == ExecutionStatus::HALTED,
          "SYSCALL did not halt the block");
  require(Context.state().ExitCode == 42,
          "Block returned an incorrect exit code");
  require(Context.cpu().PC == 16, "SYSCALL incorrectly advanced PC");
}

void testExecutionFault(const std::string &Filename, TIExecutionMode Mode) {
  ExecutionContext Context(64, Mode);
  Context.loadBinary(Filename);

  requireError(
      ErrorCode::MEMORY_OUT_OF_BOUNDS, [&] { Context.step(); },
      "Execution fault");
  require(Context.state().Status == ExecutionStatus::FAULTED,
          "Execution error did not fault the context");
}

void testJitExecutionFault(const std::string &Filename) {
  ExecutionContext Context(64, TIExecutionMode::JIT);
  Context.loadBinary(Filename);

  Context.step();
  require(Context.state().Status == ExecutionStatus::FAULTED,
          "JIT execution error did not fault the context");
  require(Context.cpu().PC == 0,
          "JIT continued execution after the execution error");
}

void testJitExecution(const std::string &Filename) {
  ExecutionContext Context(256, TIExecutionMode::JIT);
  Context.loadBinary(Filename);
  Context.run();

  const TICpuState &Cpu = Context.cpu();
  require(Context.state().Status == ExecutionStatus::HALTED,
          "JIT program did not halt");
  require(Context.state().ExitCode == 42,
          "JIT program returned an incorrect exit code");
  require(Cpu.Registers[3] == 30 && Cpu.Registers[4] == 25 &&
              Cpu.Registers[5] == 30,
          "JIT arithmetic instructions returned incorrect results");
  require(Cpu.Registers[6] == 28 && Cpu.Registers[7] == 7 &&
              Cpu.Registers[9] == 5 && Cpu.Registers[12] == 9,
          "JIT bit instructions returned incorrect results");
  require(Cpu.Registers[14] == 30 && Cpu.Registers[16] == 25,
          "JIT load instructions returned incorrect results");
  require(Context.memory().read32(136) == 10 &&
              Context.memory().read32(140) == 20,
          "JIT store pair returned incorrect results");
  require(Cpu.Registers[17] == 0,
          "JIT control flow returned an incorrect result");
  require(Cpu.PC == 96, "JIT syscall has an incorrect PC");
}

} // namespace

int main(int ArgumentCount, char *Arguments[]) {
  if (ArgumentCount != 5) {
    std::cerr << "Usage: " << Arguments[0]
              << " <cache.elf> <program.elf> <fault.elf> <jit.elf>\n";
    return 1;
  }

  try {
    testCache(Arguments[1]);
    testExecution(Arguments[2], TIExecutionMode::THREADED);
    testExecution(Arguments[2], TIExecutionMode::SWITCH);
    testExecution(Arguments[2], TIExecutionMode::JIT);
    testExecutionFault(Arguments[3], TIExecutionMode::THREADED);
    testExecutionFault(Arguments[3], TIExecutionMode::SWITCH);
    testJitExecutionFault(Arguments[3]);
    testJitExecution(Arguments[4]);
  } catch (const std::runtime_error &Error) {
    std::cerr << Error.what() << '\n';
    return 1;
  }

  std::cout << "All basic block tests passed\n";
  return 0;
}
