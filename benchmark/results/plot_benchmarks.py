#!/usr/bin/env python3
"""Plot the output produced by run_benchmarks.sh."""

from __future__ import annotations

import argparse
import re
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np


PROGRAM_RE = re.compile(r"^==\s+(.+?)\s+==$")
RESULT_RE = re.compile(
    r"^(?P<mode>threaded|switch)\s+"
    r"median=\s*(?P<median>[0-9]+(?:\.[0-9]+)?)\s+us\s+"
    r"min=\s*(?P<minimum>[0-9]+(?:\.[0-9]+)?)\s+us\s+"
    r"exit=(?P<exit>\d+)\s*$"
)


def read_results(filename: Path) -> dict[str, dict[str, dict[str, float]]]:
    results: dict[str, dict[str, dict[str, float]]] = {}
    program: str | None = None

    for line_number, line in enumerate(filename.read_text().splitlines(), 1):
        program_match = PROGRAM_RE.match(line.strip())
        if program_match:
            program = program_match.group(1)
            results.setdefault(program, {})
            continue

        result_match = RESULT_RE.match(line.strip())
        if result_match:
            if program is None:
                raise ValueError(f"Строка {line_number}: режим указан до имени программы")
            mode = result_match.group("mode")
            results[program][mode] = {
                "median": float(result_match.group("median")),
                "minimum": float(result_match.group("minimum")),
                "exit": float(result_match.group("exit")),
            }

    if not results:
        raise ValueError(f"В файле {filename} не найдено результатов бенчмарка")

    missing = {
        program: {"threaded", "switch"} - set(modes)
        for program, modes in results.items()
        if {"threaded", "switch"} - set(modes)
    }
    if missing:
        details = ", ".join(f"{name}: {sorted(modes)}" for name, modes in missing.items())
        raise ValueError(f"Для некоторых программ отсутствуют режимы: {details}")

    return results


def plot_results(results: dict[str, dict[str, dict[str, float]]], metric: str, output: Path) -> None:
    programs = list(results)
    threaded = [results[name]["threaded"][metric] for name in programs]
    switch = [results[name]["switch"][metric] for name in programs]

    positions = np.arange(len(programs))
    width = 0.36
    figure, axis = plt.subplots(figsize=(9, 5.5))
    bars_threaded = axis.bar(positions - width / 2, threaded, width, label="threaded")
    bars_switch = axis.bar(positions + width / 2, switch, width, label="switch")

    axis.set_title("TOY ISA benchmark")
    axis.set_xlabel("Программа")
    axis.set_ylabel(f"{metric} время, мкс")
    axis.set_xticks(positions, [name.removesuffix(".elf") for name in programs])
    axis.grid(axis="y", linestyle="--", alpha=0.35)
    axis.legend()

    for bars in (bars_threaded, bars_switch):
        axis.bar_label(bars, fmt="%.2f", padding=3, fontsize=8)

    figure.tight_layout()
    figure.savefig(output, dpi=160)
    print(f"График сохранён: {output}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="лог, например benchmark/result.txt")
    parser.add_argument(
        "-o", "--output", type=Path, default=Path("benchmark/benchmark.png"),
        help="PNG-файл для графика (по умолчанию: benchmark/benchmark.png)",
    )
    parser.add_argument(
        "--metric", choices=("median", "minimum"), default="median",
        help="какие значения показывать (по умолчанию: median)",
    )
    args = parser.parse_args()

    results = read_results(args.input)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    plot_results(results, args.metric, args.output)


if __name__ == "__main__":
    main()
