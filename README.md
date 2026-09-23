# TOY ISA interpreter

Интерпретатор 32-битного процессора TOY ISA v1.5 на C++17 с кэшем базовых блоков и threaded code. Описание системы команд: [`docs/TOY_ISA_5.pdf`](docs/TOY_ISA_5.pdf).

## Сборка

```bash
cmake -S . -B build -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Запуск

```bash
./build/interpreter examples/fibonacci_6.bin    # Интерпретатор

./build/run_tests    # Тестирование инструкций
```
