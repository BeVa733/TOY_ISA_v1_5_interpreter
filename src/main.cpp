#include "../include/toy/execution_context.hpp"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

constexpr std::size_t MEMORY_SIZE = 64 * 1024 * 1024;

} // namespace

int main(int ArgumentCount, char *Arguments[]) {
  if (ArgumentCount < 2 || ArgumentCount > 4) {
    std::cerr << "Usage: " << Arguments[0]
              << " <program.elf> [--engine=threaded|switch|jit]"
                 " [--jit-threshold=N]\n";
    return 1;
  }

  try {
    TIExecutionMode Mode  = TIExecutionMode::THREADED;
    uint64_t JitThreshold = 0;
    const char *Filename  = Arguments[1];

    for (int Index = 2; Index < ArgumentCount; ++Index) {
      std::string Argument = Arguments[Index];
      if (Argument == "--engine=switch") {
        Mode = TIExecutionMode::SWITCH;
      } else if (Argument == "--engine=jit") {
        Mode = TIExecutionMode::JIT;
      } else if (Argument == "--engine=threaded") {
        Mode = TIExecutionMode::THREADED;
      } else if (Argument.rfind("--jit-threshold=", 0) == 0) {
        std::string Value = Argument.substr(sizeof("--jit-threshold=") - 1);
        std::size_t ParsedCharacters = 0;
        JitThreshold                 = std::stoull(Value, &ParsedCharacters);
        if (ParsedCharacters != Value.size()) {
          throw std::runtime_error("Invalid JIT threshold: " + Value);
        }
      } else {
        throw std::runtime_error("Unknown argument: " + Argument);
      }
    }

    ExecutionContext Context(MEMORY_SIZE, Mode, JitThreshold);
    Context.loadBinary(Filename);
    Context.run();
    std::cout << "Process ended with exit code: " +
                     std::to_string(Context.state().ExitCode) + "\n";
    return 0;
  } catch (const std::exception &Error) {
    std::cerr << Error.what() << '\n';
    return 1;
  }
}
