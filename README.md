# CPU Instruction Set Benchmark

This tool benchmarks different x86 instruction sets (AVX128/256/512/AMX and basic ADD operations) on a single core while monitoring CPU frequency to determine if the processor can reach its maximum frequency while running these instructions.

## Features

- Supports testing basic integer ADD, AVX128 (SSE), AVX256, AVX512, and AMX instruction sets
- Pins execution to a single CPU core
- Monitors CPU frequency in real-time during benchmark execution
- Provides detailed frequency statistics (min, max, average)
- Gracefully handles unsupported instruction sets with fallback mechanisms
- Compatible with various x86 CPU architectures

## Building

```bash
mkdir -p build
cd build
cmake ..
make
```

## Usage

```bash
./cpu_instr_freq [options]
```

### Options

- `--help` - Show help message
- `--instr=TYPE` - Instruction set type (avx128, avx256, avx512, amx, basic_add)
- `--time=SECONDS` - Duration of the benchmark in seconds (default: 5)
- `--core=ID` - CPU core to run the benchmark on (default: 0)
- `--list` - List available CPU features and exit

### Examples

List CPU features:
```bash
./cpu_instr_freq --list
```

Run AVX256 benchmark on core 3 for 10 seconds:
```bash
./cpu_instr_freq --instr=avx256 --time=10 --core=3
```

Run AVX512 benchmark on core 0 for 5 seconds:
```bash
./cpu_instr_freq --instr=avx512
```

Run basic integer ADD benchmark on core 1 for 15 seconds:
```bash
./cpu_instr_freq --instr=basic_add --time=15 --core=1
```

## How It Works

1. The benchmark directly calls assembly instructions for the specified instruction set.
2. A separate monitoring thread records the CPU frequency of the target core at regular intervals.
3. After the benchmark completes, statistics about the observed frequencies are displayed.
4. If the CPU doesn't support the requested instruction set, a fallback implementation is used.

## Note on AVX Throttling

Many CPUs implement frequency throttling when executing AVX instructions, especially AVX-512. This tool allows you to measure and observe this behavior. On some CPUs, you might notice:

- AVX-512 instructions cause significant frequency reduction
- AVX2 instructions cause moderate frequency reduction
- SSE/AVX-128 instructions maintain higher frequencies

The AMX benchmark is implemented as a computational placeholder since AMX is a very new instruction set that requires specific hardware support.