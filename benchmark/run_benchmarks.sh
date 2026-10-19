#!/usr/bin/env bash

set -euo pipefail

ProjectDirectory="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BenchmarkExecutable="${1:-${ProjectDirectory}/build/benchmark/toy_benchmark}"
ProgramDirectory="${2:-${ProjectDirectory}/benchmark/programs/bin}"
OptimizationBenchmarkExecutable="${3:-${ProjectDirectory}/build/benchmark/toy_optimization_benchmark}"
ResultFile="${ProjectDirectory}/benchmark/results/result.txt"
PlotScript="${ProjectDirectory}/benchmark/results/plot_benchmarks.py"
PlotFile="${ProjectDirectory}/benchmark/results/result_plot.png"
OptimizationResultFile="${ProjectDirectory}/benchmark/results/optimization_result.txt"
OptimizationPlotFile="${ProjectDirectory}/benchmark/results/optimization_result_plot.png"

Programs=("${ProgramDirectory}"/*.elf)

{
  echo "JIT thresholds: 0, 1, 10"
  echo

  for Program in "${Programs[@]}"; do
    echo "== $(basename "${Program}") =="
    "${BenchmarkExecutable}" "${Program}"
    echo "=============================="
    echo
  done
} | tee "${ResultFile}"

python3 "${PlotScript}" "${ResultFile}" -o "${PlotFile}"

{
  echo "JIT optimization levels: 0, 1, 2; threshold: 1"
  echo

  for Program in "${Programs[@]}"; do
    echo "== $(basename "${Program}") =="
    "${OptimizationBenchmarkExecutable}" "${Program}"
    echo "=============================="
    echo
  done
} | tee "${OptimizationResultFile}"

python3 "${PlotScript}" "${OptimizationResultFile}" \
  --suite optimization -o "${OptimizationPlotFile}"
