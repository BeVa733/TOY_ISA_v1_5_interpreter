#!/usr/bin/env bash

set -euo pipefail

ProjectDirectory="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BenchmarkExecutable="${1:-${ProjectDirectory}/build/benchmark/toy_benchmark}"
ProgramDirectory="${2:-${ProjectDirectory}/benchmark/programs/bin}"

Programs=("${ProgramDirectory}"/*.elf)

for Program in "${Programs[@]}"; do
  echo "== $(basename "${Program}") =="
  "${BenchmarkExecutable}" "${Program}"
  echo "=============================="
  echo
done
