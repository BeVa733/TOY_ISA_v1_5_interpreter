#include "../include/toy/execution_context.hpp"

#include <cstddef>
#include <exception>
#include <iostream>

namespace {

constexpr std::size_t MEMORY_SIZE = 64 * 1024 * 1024;

} // namespace

int main(int ArgumentCount, char *Arguments[]) {
  if (ArgumentCount < 2 || ArgumentCount > 3) {
    std::cerr << "Usage: " << Arguments[0]
              << " <program.elf> [--engine=threaded|switch]\n";
    return 1;
  }

  try {
    TIExecutionMode Mode = TIExecutionMode::THREADED;
    const char *Filename = Arguments[1];
    if (ArgumentCount == 3) {
      std::string Engine = Arguments[2];
      if (Engine == "--engine=switch") {
        Mode = TIExecutionMode::SWITCH;
      } else if (Engine != "--engine=threaded") {
        throw std::runtime_error("Unknown execution engine: " + Engine);
      }
    }

    ExecutionContext Context(MEMORY_SIZE, Mode);
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
