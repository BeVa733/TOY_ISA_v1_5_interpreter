#pragma once

#include "instruction.hpp"

class ExecutionContext;

void executeLi      (ExecutionContext &, const TIInstruction &);
void executeAdd     (ExecutionContext &, const TIInstruction &);
void executeAddi    (ExecutionContext &, const TIInstruction &);
void executeOr      (ExecutionContext &, const TIInstruction &);
void executeLdReg   (ExecutionContext &, const TIInstruction &);
void executeLdImm   (ExecutionContext &, const TIInstruction &);
void executeSt      (ExecutionContext &, const TIInstruction &);
void executeStp     (ExecutionContext &, const TIInstruction &);
void executeBeq     (ExecutionContext &, const TIInstruction &);
void executeJ       (ExecutionContext &, const TIInstruction &);
void executeClz     (ExecutionContext &, const TIInstruction &);
void executeSsat    (ExecutionContext &, const TIInstruction &);
void executeRori    (ExecutionContext &, const TIInstruction &);
void executeBext    (ExecutionContext &, const TIInstruction &);
void executeSyscall (ExecutionContext &, const TIInstruction &);
void executeInvalid (ExecutionContext &, const TIInstruction &);
