#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

enum class ExecutionStatus : uint8_t {
  RUNNING = 0,
  HALTED  = 1,
  FAULTED = 2
};

struct TIExecutionState {
  ExecutionStatus Status = ExecutionStatus::RUNNING;
  uint32_t ExitCode      = 0;

  void reset() {
    Status   = ExecutionStatus::RUNNING;
    ExitCode = 0;
  }

  void halt(uint32_t Code) {
    Status   = ExecutionStatus::HALTED;
    ExitCode = Code;
  }

  void fault() { Status = ExecutionStatus::FAULTED; }
};

enum class ErrorCode : uint8_t {
  INVALID_INSTRUCTION   = 0,
  MEMORY_OUT_OF_BOUNDS  = 1,
  MISALIGNED_ACCESS     = 2,
  UNSUPPORTED_SYSCALL   = 3,
  INCORRECT_BINARY_FILE = 4
};

class SimulationException : public std::runtime_error {
public:
  SimulationException(ErrorCode Code, const std::string &Messasge)
      : std::runtime_error(Messasge), Code_(Code) {}

  ErrorCode code() const { return Code_; }

private:
  ErrorCode Code_ = ErrorCode::INVALID_INSTRUCTION;
};
