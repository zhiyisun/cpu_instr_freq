#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>

// CPU core-related functions
void pin_to_core(int core_id);
int get_core_count();
int get_max_core_id();

// CPU frequency monitoring
double get_cpu_freq_mhz(int core_id);
std::vector<double> monitor_cpu_freq(int core_id, int duration_ms, int sampling_interval_ms);
std::map<int, double> get_all_core_frequencies(); // New function to get all core frequencies
std::map<int, std::vector<double>> monitor_all_cpu_freq(int duration_ms, int sampling_interval_ms); // New function to monitor all cores

// Run a function on a specific core
void run_on_core(int core_id, const std::function<void()>& func);

// Run a function on all cores in parallel
void run_on_all_cores(const std::function<void()>& func);

// Run a function on all cores sequentially
void run_on_all_cores_sequential(const std::function<void(int)>& func);

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
void print_all_core_frequencies();
void print_single_core_info(int core_id); // New function to print info for just one core