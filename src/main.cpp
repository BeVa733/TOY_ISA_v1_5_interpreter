#include "../include/toy/execution_context.hpp"

#include <cstddef>
#include <exception>
#include <iostream>

namespace {

constexpr std::size_t MEMORY_SIZE = 64 * 1024 * 1024;

} // namespace

int main(int ArgumentCount, char *Arguments[]) {
  if (ArgumentCount != 2) {
    std::cerr << "Usage: " << Arguments[0] << " <binary>\n";
    return 1;
  }

  try {
    ExecutionContext Context(MEMORY_SIZE);
    Context.loadBinary(Arguments[1]);
    Context.run();
    std::cout << "Process ended with exit code: " +
                     std::to_string(Context.state().ExitCode) + "\n";
    return 0;
  } catch (const std::exception &Error) {
    std::cerr << Error.what() << '\n';
    return 1;
  }
}
