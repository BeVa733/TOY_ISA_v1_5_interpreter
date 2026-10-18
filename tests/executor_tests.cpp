#include "../include/execution_type/threaded_executor.hpp"
#include "../include/toy/execution_context.hpp"
#include "test_utils.hpp"

#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {

void executeInstruction(ExecutionContext &Context, TIInstruction Instruction) {
  Instruction.ThreadedHandler = getThreadedHandler(Instruction.OpCode);

  Instruction.ThreadedHandler(Context, &Instruction, &Instruction + 1);
}

void testLi() {
  ExecutionContext Context(64);
  TICpuState &Cpu = Context.cpu();

  executeInstruction(
      Context, {
                   TIOpcode::LI,
                   {{OperandType::REGISTER, 31},
                         {OperandType::IMMEDIATE, static_cast<uint32_t>(-1)}}
  });

  require(Cpu.Registers[31] == 0xFFFFFFFF, "LI failed");
  require(Cpu.PC == TI32::INSTRUCTION_SIZE, "LI did not advance PC");
}

void testAdd() {
  ExecutionContext Context(64);
  TICpuState &Cpu  = Context.cpu();
  Cpu.Registers[1] = 0xFFFFFFFF;
  Cpu.Registers[2] = 1;

  executeInstruction(Context, {
                                  TIOpcode::ADD,
                                  {{OperandType::REGISTER, 3},
                                         {OperandType::REGISTER, 1},
                                         {OperandType::REGISTER, 2}}
  });

  require(Cpu.Registers[3] == 0, "ADD wraparound failed");

  ExecutionContext RegularContext(64);
  TICpuState &Regular  = RegularContext.cpu();
  Regular.Registers[1] = 10;
  Regular.Registers[2] = 20;
  executeInstruction(RegularContext, {
                                         TIOpcode::ADD,
                                         {{OperandType::REGISTER, 3},
                                                {OperandType::REGISTER, 1},
                                                {OperandType::REGISTER, 2}}
  });
  require(Regular.Registers[3] == 30, "ADD regular operation failed");
}

void testAddi() {
  ExecutionContext Context(64);
  TICpuState &Cpu  = Context.cpu();
  Cpu.Registers[1] = 5;

  executeInstruction(
      Context, {
                   TIOpcode::ADDI,
                   {{OperandType::REGISTER, 2},
                           {OperandType::REGISTER, 1},
                           {OperandType::IMMEDIATE, static_cast<uint32_t>(-7)}}
  });

  require(Cpu.Registers[2] == 0xFFFFFFFE, "ADDI failed");

  ExecutionContext RegularContext(64);
  TICpuState &Regular  = RegularContext.cpu();
  Regular.Registers[1] = 5;
  executeInstruction(RegularContext, {
                                         TIOpcode::ADDI,
                                         {{OperandType::REGISTER, 2},
                                                 {OperandType::REGISTER, 1},
                                                 {OperandType::IMMEDIATE, 7}}
  });
  require(Regular.Registers[2] == 12, "ADDI regular operation failed");
}

void testOr() {
  ExecutionContext Context(64);
  TICpuState &Cpu  = Context.cpu();
  Cpu.Registers[1] = 0x0F00;
  Cpu.Registers[2] = 0x00F0;

  executeInstruction(Context, {
                                  TIOpcode::OR,
                                  {{OperandType::REGISTER, 3},
                                        {OperandType::REGISTER, 1},
                                        {OperandType::REGISTER, 2}}
  });

  require(Cpu.Registers[3] == 0x0FF0, "OR failed");
}

void testSt() {
  ExecutionContext Context(128);
  TICpuState &Cpu  = Context.cpu();
  TIMemory &Memory = Context.memory();
  Cpu.Registers[1] = 80;
  Cpu.Registers[2] = 0x1234;

  executeInstruction(
      Context, {
                   TIOpcode::ST,
                   {{OperandType::REGISTER, 2},
                         {OperandType::REGISTER, 1},
                         {OperandType::IMMEDIATE, static_cast<uint32_t>(-4)}}
  });

  require(Memory.read32(76) == 0x1234, "ST failed");
}

void testLdImmediate() {
  ExecutionContext Context(128);
  TICpuState &Cpu  = Context.cpu();
  TIMemory &Memory = Context.memory();
  Cpu.Registers[1] = 80;
  Memory.write32(76, 0x2345);

  executeInstruction(
      Context, {
                   TIOpcode::LDimm,
                   {{OperandType::REGISTER, 3},
                            {OperandType::REGISTER, 1},
                            {OperandType::IMMEDIATE, static_cast<uint32_t>(-4)}}
  });

  require(Cpu.Registers[3] == 0x2345, "LD immediate failed");
}

void testLdRegister() {
  ExecutionContext Context(128);
  TICpuState &Cpu  = Context.cpu();
  TIMemory &Memory = Context.memory();
  Cpu.Registers[1] = 80;
  Cpu.Registers[3] = 4;
  Memory.write32(84, 0x3456);

  executeInstruction(Context, {
                                  TIOpcode::LDreg,
                                  {{OperandType::REGISTER, 4},
                                           {OperandType::REGISTER, 1},
                                           {OperandType::REGISTER, 3}}
  });

  require(Cpu.Registers[4] == 0x3456, "LD register failed");
}

void testStp() {
  ExecutionContext Context(128);
  TICpuState &Cpu  = Context.cpu();
  TIMemory &Memory = Context.memory();
  Cpu.Registers[1] = 80;
  Cpu.Registers[2] = 0x1111;
  Cpu.Registers[3] = 0x2222;

  executeInstruction(Context, {
                                  TIOpcode::STP,
                                  {{OperandType::REGISTER, 2},
                                         {OperandType::REGISTER, 3},
                                         {OperandType::REGISTER, 1},
                                         {OperandType::IMMEDIATE, 4}}
  });

  require(Memory.read32(84) == 0x1111, "STP first word failed");
  require(Memory.read32(88) == 0x2222, "STP second word failed");
}

void testBeq() {
  ExecutionContext TakenContext(64);
  TICpuState &Taken  = TakenContext.cpu();
  Taken.PC           = 8;
  Taken.Registers[1] = 7;
  Taken.Registers[2] = 7;
  executeInstruction(TakenContext, {
                                       TIOpcode::BEQ,
                                       {{OperandType::REGISTER, 1},
                                              {OperandType::REGISTER, 2},
                                              {OperandType::IMMEDIATE, 2}}
  });
  require(Taken.PC == 16, "BEQ taken failed");

  ExecutionContext NotTakenContext(64);
  TICpuState &NotTaken  = NotTakenContext.cpu();
  NotTaken.PC           = 8;
  NotTaken.Registers[1] = 7;
  NotTaken.Registers[2] = 8;
  executeInstruction(NotTakenContext, {
                                          TIOpcode::BEQ,
                                          {{OperandType::REGISTER, 1},
                                                 {OperandType::REGISTER, 2},
                                                 {OperandType::IMMEDIATE, 2}}
  });
  require(NotTaken.PC == 12, "BEQ not taken failed");

  ExecutionContext BackwardContext(64);
  TICpuState &Backward  = BackwardContext.cpu();
  Backward.PC           = 8;
  Backward.Registers[1] = 7;
  Backward.Registers[2] = 7;
  executeInstruction(BackwardContext, {
                                          TIOpcode::BEQ,
                                          {{OperandType::REGISTER, 1},
                                                 {OperandType::REGISTER, 2},
                                                 {OperandType::IMMEDIATE,
                                            static_cast<uint32_t>(-1)}}
  });
  require(Backward.PC == 4, "BEQ backward branch failed");
}

void testJ() {
  ExecutionContext Context(64);
  TICpuState &Cpu = Context.cpu();
  Cpu.PC          = 0xA0000000;

  executeInstruction(Context, {TIOpcode::J, {{OperandType::IMMEDIATE, 5}}});

  require(Cpu.PC == 0xA0000014, "J failed");
}

void testClz() {
  ExecutionContext Context(64);
  TICpuState &Cpu = Context.cpu();

  executeInstruction(
      Context, {
                   TIOpcode::CLZ,
                   {{OperandType::REGISTER, 1}, {OperandType::REGISTER, 0}}
  });
  Cpu.Registers[2] = 1;
  executeInstruction(
      Context, {
                   TIOpcode::CLZ,
                   {{OperandType::REGISTER, 3}, {OperandType::REGISTER, 2}}
  });
  Cpu.Registers[4] = 0xFFFFFFFF;
  executeInstruction(
      Context, {
                   TIOpcode::CLZ,
                   {{OperandType::REGISTER, 5}, {OperandType::REGISTER, 4}}
  });

  require(Cpu.Registers[1] == TI32::WORD_BIT_COUNT, "CLZ zero failed");
  require(Cpu.Registers[3] == 31, "CLZ one failed");
  require(Cpu.Registers[5] == 0, "CLZ full reg failed");
}

void testSsat() {
  ExecutionContext Context(64);
  TICpuState &Cpu  = Context.cpu();
  Cpu.Registers[1] = 200;
  Cpu.Registers[3] = static_cast<uint32_t>(-200);
  Cpu.Registers[5] = 50;

  executeInstruction(Context, {
                                  TIOpcode::SSAT,
                                  {{OperandType::REGISTER, 2},
                                          {OperandType::REGISTER, 1},
                                          {OperandType::IMMEDIATE, 8}}
  });
  executeInstruction(Context, {
                                  TIOpcode::SSAT,
                                  {{OperandType::REGISTER, 4},
                                          {OperandType::REGISTER, 3},
                                          {OperandType::IMMEDIATE, 8}}
  });
  executeInstruction(Context, {
                                  TIOpcode::SSAT,
                                  {{OperandType::REGISTER, 6},
                                          {OperandType::REGISTER, 5},
                                          {OperandType::IMMEDIATE, 8}}
  });

  require(Cpu.Registers[2] == 127, "SSAT upper bound failed");
  require(Cpu.Registers[4] == 0xFFFFFF80, "SSAT lower bound failed");
  require(Cpu.Registers[6] == 50, "SSAT changed value inside range");
}

void testRori() {
  ExecutionContext Context(64);
  TICpuState &Cpu  = Context.cpu();
  Cpu.Registers[1] = 1;

  executeInstruction(Context, {
                                  TIOpcode::RORI,
                                  {{OperandType::REGISTER, 2},
                                          {OperandType::REGISTER, 1},
                                          {OperandType::IMMEDIATE, 1}}
  });
  executeInstruction(Context, {
                                  TIOpcode::RORI,
                                  {{OperandType::REGISTER, 3},
                                          {OperandType::REGISTER, 1},
                                          {OperandType::IMMEDIATE, 0}}
  });
  executeInstruction(Context, {
                                  TIOpcode::RORI,
                                  {{OperandType::REGISTER, 4},
                                          {OperandType::REGISTER, 1},
                                          {OperandType::IMMEDIATE, 31}}
  });

  require(Cpu.Registers[2] == 0x80000000, "RORI one failed");
  require(Cpu.Registers[3] == 1, "RORI zero failed");
  require(Cpu.Registers[4] == 2, "RORI 31 failed");
}

void testBext() {
  ExecutionContext Context(64);
  TICpuState &Cpu  = Context.cpu();
  Cpu.Registers[1] = 0x00D6;
  Cpu.Registers[2] = 0x00AC;

  executeInstruction(Context, {
                                  TIOpcode::BEXT,
                                  {{OperandType::REGISTER, 3},
                                          {OperandType::REGISTER, 1},
                                          {OperandType::REGISTER, 2}}
  });

  require(Cpu.Registers[3] == 9, "BEXT failed");
}

void testSyscall() {
  ExecutionContext Context(64);
  TICpuState &Cpu         = Context.cpu();
  TIExecutionState &State = Context.state();
  Cpu.Registers[8]        = 60;
  Cpu.Registers[0]        = 42;

  executeInstruction(Context, {TIOpcode::SYSCALL, {}});

  require(State.Status == ExecutionStatus::HALTED, "exit did not halt");
  require(State.ExitCode == 42, "exit returned incorrect code");

  ExecutionContext UnsupportedContext(64);
  TICpuState &UnsupportedCpu  = UnsupportedContext.cpu();
  UnsupportedCpu.Registers[8] = 999;

  requireError(
      ErrorCode::UNSUPPORTED_SYSCALL,
      [&] { executeInstruction(UnsupportedContext, {TIOpcode::SYSCALL, {}}); },
      "Unsupported syscall");
}

} // namespace

int main() {
  try {
    testLi();
    testAdd();
    testAddi();
    testOr();
    testSt();
    testLdImmediate();
    testLdRegister();
    testStp();
    testBeq();
    testJ();
    testClz();
    testSsat();
    testRori();
    testBext();
    testSyscall();
  } catch (const std::runtime_error &Error) {
    std::cerr << Error.what() << '\n';
    return 1;
  }

  std::cout << "All executor tests passed\n";
  return 0;
}
