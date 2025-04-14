#pragma once

#include <string>
#include <vector>

enum class InstructionSet {
    AVX128,
    AVX256,
    AVX512,
    AMX,
    BASIC_ADD
};

// Structure to hold benchmark results
struct BenchmarkResult {
    int core_id;
    double min_freq;
    double max_freq;
    double avg_freq;
    std::vector<double> frequencies;
    bool success;
};

// Convert string to instruction set enum
InstructionSet string_to_instruction_set(const std::string& str);

// Run the benchmark with specified instruction set for the given duration
void run_benchmark(InstructionSet instr_set, int duration_sec, int core_id);

// Run the benchmark with specified instruction set and return results
BenchmarkResult run_benchmark_with_result(InstructionSet instr_set, int duration_sec, int core_id);

// Get the string name of the instruction set
std::string get_instruction_set_name(InstructionSet instr_set);

// Print detailed benchmark results
void print_benchmark_result(const BenchmarkResult& result, const std::string& instr_name);