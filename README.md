# TOY ISA interpreter

Интерпретатор 32-битного процессора TOY ISA v1.5 на C++17 с кэшем базовых блоков, switch-, threaded- и JIT-исполнением. Описание системы команд: [`docs/TOY_ISA_5.pdf`](docs/TOY_ISA_5.pdf). Примеры ассемблерных и ELF-программ: [`examples/asm/`](examples/asm/) и [`examples/bin/`](examples/bin/).

## Быстрый старт

```bash
git clone --recurse-submodules git@github.com:BeVa733/TOY_ISA_v1_5_interpreter.git
cd TOY_ISA_v1_5_interpreter
cmake -S . -B build -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Запуск

```bash
# Ассемблер
./assembler/toy_as.rb <source.rb> [programm.elf]

# Интерпретатор
./build/interpreter <program.elf> [--engine=threaded|switch|jit] \
  [--jit-threshold=N] [--jit-opt-level=0|1|2]

# Прогон всех доступных тестов
ctest --test-dir build

# Запуск бенчмарков сравнения switch диспетчеризации, thraded code и JIT компилляции 
chmod +x benchmark/run_benchmarks.sh
./benchmark/run_benchmarks.sh
```
