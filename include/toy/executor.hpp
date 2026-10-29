#pragma once

#include "execution_context.hpp"
#include "instruction.hpp"

namespace ExecutorDetail {

uint32_t countLeadingZeros(uint32_t Value);
uint32_t saturateSigned   (uint32_t Value, uint32_t Width);
uint32_t rotateRight      (uint32_t Value, uint32_t Shift);
uint32_t compressBits     (uint32_t Value, uint32_t Mask);

} // namespace ExecutorDetail

void executeLi     (ExecutionContext &Context, const TIInstruction &Instruction);
void executeAdd    (ExecutionContext &Context, const TIInstruction &Instruction);
void executeAddi   (ExecutionContext &Context, const TIInstruction &Instruction);
void executeOr     (ExecutionContext &Context, const TIInstruction &Instruction);
void executeLdReg  (ExecutionContext &Context, const TIInstruction &Instruction);
void executeLdImm  (ExecutionContext &Context, const TIInstruction &Instruction);
void executeSt     (ExecutionContext &Context, const TIInstruction &Instruction);
void executeStSmc  (ExecutionContext &Context, const TIInstruction &Instruction);
void executeStp    (ExecutionContext &Context, const TIInstruction &Instruction);
void executeStpSmc (ExecutionContext &Context, const TIInstruction &Instruction);
void executeBeq    (ExecutionContext &Context, const TIInstruction &Instruction);
void executeJ      (ExecutionContext &Context, const TIInstruction &Instruction);
void executeClz    (ExecutionContext &Context, const TIInstruction &Instruction);
void executeSsat   (ExecutionContext &Context, const TIInstruction &Instruction);
void executeRori   (ExecutionContext &Context, const TIInstruction &Instruction);
void executeBext   (ExecutionContext &Context, const TIInstruction &Instruction);
void executeSyscall(ExecutionContext &Context,
                    const TIInstruction &Instruction);
void executeInvalid(ExecutionContext &Context,
                    const TIInstruction &Instruction);
