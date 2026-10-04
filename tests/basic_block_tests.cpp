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

  const TIMemory &Memory = Context.memory();
  TIDecoder Decoder{};
  TIBasicBlockCache Cache{};

  TIBasicBlock &Jump = Cache.getBlock(0, Memory, Decoder);
  require(Jump.startAddress() == 0, "J block has incorrect start address");
  require(Jump.instructions().size() == 3, "J did not end its block");
  require(Jump.instructions().back().OpCode == TIOpcode::J,
          "J block has incorrect last instruction");
  require(&Cache.getBlock(0, Memory, Decoder) == &Jump,
          "Cached block was decoded again");

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
}

void testThreadedExecution(const std::string &Filename) {
  ExecutionContext Context(64);
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

void testExecutionFault(const std::string &Filename) {
  ExecutionContext Context(64);
  Context.loadBinary(Filename);

  requireError(
      ErrorCode::MEMORY_OUT_OF_BOUNDS, [&] { Context.step(); },
      "Execution fault");
  require(Context.state().Status == ExecutionStatus::FAULTED,
          "Execution error did not fault the context");
}

} // namespace

int main(int ArgumentCount, char *Arguments[]) {
  if (ArgumentCount != 4) {
    std::cerr << "Usage: " << Arguments[0]
              << " <cache.bin> <program.bin> <fault.bin>\n";
    return 1;
  }

  try {
    testCache(Arguments[1]);
    testThreadedExecution(Arguments[2]);
    testExecutionFault(Arguments[3]);
  } catch (const std::runtime_error &Error) {
    std::cerr << Error.what() << '\n';
    return 1;
  }

  std::cout << "All basic block tests passed\n";
  return 0;
}
