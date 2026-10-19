#!/usr/bin/env python3
"""Plot the output produced by run_benchmarks.sh."""

from __future__ import annotations

import argparse
import re
from pathlib import Path

import matplotlib.pyplot as plt


PROGRAM_RE = re.compile(r"^==\s+(.+?)\s+==$")
RESULT_RE = re.compile(
    r"^(?P<mode>threaded|switch|jit-t0|jit-t1|jit-t10|jit-o0|jit-o1|jit-o2)\s+"
    r"median=\s*(?P<median>[0-9]+(?:\.[0-9]+)?)\s+us\s+"
    r"min=\s*(?P<minimum>[0-9]+(?:\.[0-9]+)?)\s+us\s+"
    r"exit=(?P<exit>\d+)\s*$"
)
MODES = ("threaded", "switch", "jit-t0", "jit-t1", "jit-t10")
OPTIMIZATION_MODES = ("jit-o0", "jit-o1", "jit-o2")


def read_results(
    filename: Path, modes: tuple[str, ...]
) -> dict[str, dict[str, dict[str, float]]]:
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

    expected_modes = set(modes)
    missing = {
        program: expected_modes - set(modes)
        for program, modes in results.items()
        if expected_modes - set(modes)
    }
    if missing:
        details = ", ".join(f"{name}: {sorted(modes)}" for name, modes in missing.items())
        raise ValueError(f"Для некоторых программ отсутствуют режимы: {details}")

    return results


def plot_results(
    results: dict[str, dict[str, dict[str, float]]],
    modes: tuple[str, ...],
    metric: str,
    output: Path,
) -> None:
    programs = list(results)
    figure, axes = plt.subplots(
        1, len(programs), figsize=(4.5 * len(programs), 5.5), squeeze=False
    )
    colors = plt.rcParams["axes.prop_cycle"].by_key()["color"][:len(modes)]

    for axis, program in zip(axes[0], programs):
        values = [results[program][mode][metric] for mode in modes]
        bars = axis.bar(modes, values, color=colors)
        axis.bar_label(bars, fmt="%.2f", padding=3, fontsize=8)
        axis.set_title(program.removesuffix(".elf"))
        axis.set_ylabel(f"{metric}, мкс")
        axis.tick_params(axis="x", rotation=30)
        axis.grid(axis="y", linestyle="--", alpha=0.35)

    figure.suptitle("TOY ISA benchmark")

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
    parser.add_argument(
        "--suite", choices=("execution", "optimization"), default="execution",
        help="набор режимов для графика (по умолчанию: execution)",
    )
    args = parser.parse_args()

    modes = MODES if args.suite == "execution" else OPTIMIZATION_MODES
    results = read_results(args.input, modes)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    plot_results(results, modes, args.metric, args.output)


if __name__ == "__main__":
    main()
