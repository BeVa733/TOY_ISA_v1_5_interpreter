#include "../include/toy/execution_context.hpp"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

constexpr std::size_t MEMORY_SIZE = 64 * 1024 * 1024;

struct CommandLineOptions final {
  const char *Filename         = nullptr;
  TIExecutionMode Mode         = TIExecutionMode::THREADED;
  uint64_t JitThreshold        = 0;
  uint8_t JitOptimizationLevel = 0;
};

CommandLineOptions parseCommandLine(int ArgumentCount, char *Arguments[]) {
  if (ArgumentCount < 2 || ArgumentCount > 5) {
    throw std::runtime_error(std::string("Usage: ") + Arguments[0] +
                             " <program.elf> [--engine=threaded|switch|jit]"
                             " [--jit-threshold=N] [--jit-opt-level=0|1|2]");
  }

  CommandLineOptions Options{};
  Options.Filename = Arguments[1];

  for (int Index = 2; Index < ArgumentCount; ++Index) {
    std::string Argument = Arguments[Index];
    if (Argument == "--engine=switch") {
      Options.Mode = TIExecutionMode::SWITCH;
    } else if (Argument == "--engine=jit") {
      Options.Mode = TIExecutionMode::JIT;
    } else if (Argument == "--engine=threaded") {
      Options.Mode = TIExecutionMode::THREADED;
    } else if (Argument.rfind("--jit-threshold=", 0) == 0) {
      std::string Value = Argument.substr(sizeof("--jit-threshold=") - 1);
      std::size_t ParsedCharacters = 0;
      Options.JitThreshold         = std::stoull(Value, &ParsedCharacters);
      if (ParsedCharacters != Value.size()) {
        throw std::runtime_error("Invalid JIT threshold: " + Value);
      }
    } else if (Argument.rfind("--jit-opt-level=", 0) == 0) {
      std::string Value = Argument.substr(sizeof("--jit-opt-level=") - 1);
      std::size_t ParsedCharacters = 0;
      const uint64_t ParsedLevel   = std::stoull(Value, &ParsedCharacters);
      if (ParsedCharacters != Value.size() || ParsedLevel > 2) {
        throw std::runtime_error("Invalid JIT optimization level: " + Value);
      }
      Options.JitOptimizationLevel = static_cast<uint8_t>(ParsedLevel);
    } else {
      throw std::runtime_error("Unknown argument: " + Argument);
    }
  }

  return Options;
}

} // namespace

int main(int ArgumentCount, char *Arguments[]) {
  try {
    const CommandLineOptions Options =
        parseCommandLine(ArgumentCount, Arguments);

    ExecutionContext Context(MEMORY_SIZE, Options.Mode, Options.JitThreshold,
                             Options.JitOptimizationLevel);
    Context.loadBinary(Options.Filename);
    Context.run();
    std::cout << "Process ended with exit code: " +
                     std::to_string(Context.state().ExitCode) + "\n";
    return 0;
  } catch (const std::exception &Error) {
    std::cerr << Error.what() << '\n';
    return 1;
  }
}
