#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

enum class ExecutionStatus : uint8_t {
  RUNNING = 0,
  HALTED  = 1,
  FAULTED = 2
};

struct ExecutionState {
  ExecutionStatus Status = ExecutionStatus::RUNNING;
  uint32_t ExitCode      = 0;

  void reset();
  void halt(uint32_t Code);
  void fault();
};

enum class ErrorCode : uint8_t {
  INVALID_INSTRUCTION  = 0,
  MEMORY_OUT_OF_BOUNDS = 1,
  MISALIGNED_ACCESS    = 2,
  UNSUPPORTED_SYSCALL  = 3
};

class SimulationException : public std::runtime_error {
public:
  SimulationException(ErrorCode Code, const std::string &Messasge)
      : std::runtime_error(Messasge), Code_(Code) {}

  ErrorCode code() const { return Code_; }

private:
  ErrorCode Code_ = ErrorCode::INVALID_INSTRUCTION;
};
