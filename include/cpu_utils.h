#pragma once

#include <string>
#include <vector>

// CPU core-related functions
void pin_to_core(int core_id);
int get_core_count();
int get_max_core_id();

// CPU frequency monitoring
double get_cpu_freq_mhz(int core_id);
std::vector<double> monitor_cpu_freq(int core_id, int duration_ms, int sampling_interval_ms);

// CPU feature detection
bool has_sse();
bool has_sse2();
bool has_avx();
bool has_avx2();
bool has_avx512f();
bool has_amx();

// CPUID helper
void safe_cpuid(unsigned int leaf, unsigned int subleaf, unsigned int* eax, unsigned int* ebx, unsigned int* ecx, unsigned int* edx);

// Print CPU information
void print_cpu_info();