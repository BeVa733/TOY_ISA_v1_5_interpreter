# TOY ISA interpreter

Интерпретатор 32-битного процессора TOY ISA v1.5 на C++17 с кэшем базовых блоков и threaded code. Описание системы команд: [`docs/TOY_ISA_5.pdf`](docs/TOY_ISA_5.pdf). Примеры ассемблерных и бинарных программ: [`examples/asm/`](examples/asm/) и [`examples/bin/`](examples/bin/)

## Сборка

```bash
cmake -S . -B build -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
cmake --build build
chmod +x assembler/toy_as.rb
```

## Запуск

```bash
./assembler/toy_as.rb <source_filename> [binary_filename] # ассемблер

./build/interpreter <source_binary_filename>    # Интерпретатор

./build/run_tests    # Тестирование инструкций
```
