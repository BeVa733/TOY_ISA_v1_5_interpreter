#!/usr/bin/env bash

set -euo pipefail

ProjectDirectory="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BenchmarkExecutable="${1:-${ProjectDirectory}/build/benchmark/toy_benchmark}"
ProgramDirectory="${2:-${ProjectDirectory}/benchmark/programs/bin}"
ResultFile="${ProjectDirectory}/benchmark/results/result.txt"
PlotScript="${ProjectDirectory}/benchmark/results/plot_benchmarks.py"
PlotFile="${ProjectDirectory}/benchmark/results/result_plot.png"

Programs=("${ProgramDirectory}"/*.elf)

{
  for Program in "${Programs[@]}"; do
    echo "== $(basename "${Program}") =="
    "${BenchmarkExecutable}" "${Program}"
    echo "=============================="
    echo
  done
} | tee "${ResultFile}"

python3 "${PlotScript}" "${ResultFile}" -o "${PlotFile}"
