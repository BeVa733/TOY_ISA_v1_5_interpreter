#include "toy/execution_context.hpp"
#include "toy/toy_constants.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {

constexpr std::size_t MEMORY_SIZE  = TI32::WORD_BIT_COUNT * 1024;
constexpr std::size_t SAMPLE_COUNT = 50;

struct BenchmarkResult final {
  std::vector<double> Samples{};
  uint32_t ExitCode = 0;
};

struct BenchmarkCase final {
  const char *Name      = nullptr;
  TIExecutionMode Mode  = TIExecutionMode::THREADED;
  uint64_t JitThreshold = 0;
  BenchmarkResult Result{};
};

void measureSample(const std::string &Filename, TIExecutionMode Mode,
                   uint64_t JitThreshold, BenchmarkResult &Result) {
  ExecutionContext Context(MEMORY_SIZE, Mode, JitThreshold);
  Context.loadBinary(Filename);

  const auto Start = std::chrono::steady_clock::now();
  Context.run();
  const auto End = std::chrono::steady_clock::now();

  if (Context.state().Status != ExecutionStatus::HALTED) {
    throw std::runtime_error("Benchmark program did not halt");
  }

  Result.ExitCode = Context.state().ExitCode;
  Result.Samples.push_back(
      std::chrono::duration<double, std::micro>(End - Start).count());
}

double median(std::vector<double> Samples) {
  std::sort(Samples.begin(), Samples.end());
  return Samples[Samples.size() / 2];
}

double minimum(const std::vector<double> &Samples) {
  return *std::min_element(Samples.begin(), Samples.end());
}

void printResult(const char *Name, const BenchmarkResult &Result) {
  std::cout << std::left << std::setw(9) << Name << " median=" << std::right
            << std::fixed << std::setprecision(2) << std::setw(10)
            << median(Result.Samples) << " us"
            << " min=" << std::setw(10) << minimum(Result.Samples) << " us"
            << " exit=" << Result.ExitCode << '\n';
}

} // namespace

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <program.elf>\n";
    return 1;
  }

  try {
    const std::string Filename = argv[1];
    std::array<BenchmarkCase, 5> Cases{
        {
         {"threaded", TIExecutionMode::THREADED, 0, {}},
         {"switch", TIExecutionMode::SWITCH, 0, {}},
         {"jit-t0", TIExecutionMode::JIT, 0, {}},
         {"jit-t1", TIExecutionMode::JIT, 1, {}},
         {"jit-t10", TIExecutionMode::JIT, 10, {}},
         }
    };

    for (BenchmarkCase &Case : Cases) {
      Case.Result.Samples.reserve(SAMPLE_COUNT);
    }

    for (std::size_t Iteration = 0; Iteration < SAMPLE_COUNT; ++Iteration) {
      for (std::size_t Offset = 0; Offset < Cases.size(); ++Offset) {
        BenchmarkCase &Case = Cases[(Iteration + Offset) % Cases.size()];
        measureSample(Filename, Case.Mode, Case.JitThreshold, Case.Result);
      }
    }

    for (const BenchmarkCase &Case : Cases) {
      if (Case.Result.ExitCode != Cases.front().Result.ExitCode) {
        throw std::runtime_error(
            "Execution modes returned different exit codes");
      }
    }

    for (const BenchmarkCase &Case : Cases) {
      printResult(Case.Name, Case.Result);
    }
  } catch (const std::exception &Error) {
    std::cerr << Error.what() << '\n';
    return 1;
  }

  return 0;
}
