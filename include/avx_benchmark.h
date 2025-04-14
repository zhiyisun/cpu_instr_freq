#pragma once

#include <string>

enum class InstructionSet {
    AVX128,
    AVX256,
    AVX512,
    AMX,
    BASIC_ADD
};

// Convert string to instruction set enum
InstructionSet string_to_instruction_set(const std::string& str);

// Run the benchmark with specified instruction set for the given duration
void run_benchmark(InstructionSet instr_set, int duration_sec, int core_id);