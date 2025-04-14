#include "cpu_utils.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <regex>
#include <pthread.h>
#include <map>
#include <mutex>
#include <functional>

// Use a more cautious approach for cpuid
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
#include <cpuid.h>
#define HAS_CPUID 1
#else
#define HAS_CPUID 0
#endif

// Safe CPUID implementation with signal protection
void safe_cpuid(unsigned int leaf, unsigned int subleaf, unsigned int* eax, unsigned int* ebx, unsigned int* ecx, unsigned int* edx) {
    // Initialize with safe values
    *eax = 0;
    *ebx = 0;
    *ecx = 0;
    *edx = 0;
    
#if HAS_CPUID
    // Get maximum supported CPUID leaf
    unsigned int max_leaf = 0;
    if (__get_cpuid(0, &max_leaf, ebx, ecx, edx)) {
        // Only call CPUID if the leaf is supported
        if (leaf <= max_leaf) {
            if (leaf == 7) {
                __get_cpuid_count(leaf, subleaf, eax, ebx, ecx, edx);
            } else {
                __get_cpuid(leaf, eax, ebx, ecx, edx);
            }
        }
    }
#endif
}

void pin_to_core(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    
    pthread_t current_thread = pthread_self();
    int result = pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
    
    if (result != 0) {
        std::cerr << "Error pinning thread to core " << core_id << std::endl;
        exit(1);
    }
    
    std::cout << "Pinned to core " << core_id << std::endl;
}

int get_core_count() {
    return std::thread::hardware_concurrency();
}

int get_max_core_id() {
    return get_core_count() - 1;
}

// Read CPU frequency from /proc/cpuinfo for a specific core
double get_cpu_freq_mhz(int core_id) {
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line;
    std::regex cpu_mhz_regex("^cpu MHz\\s+:\\s+([0-9]+\\.[0-9]+)$");
    std::smatch match;

    int current_core = -1;
    double frequency = 0.0;

    while (std::getline(cpuinfo, line)) {
        if (line.find("processor") != std::string::npos) {
            std::string core_str = line.substr(line.find(":") + 1);
            core_str.erase(0, core_str.find_first_not_of(" \t"));
            current_core = std::stoi(core_str);
        }
        
        if (current_core == core_id && std::regex_search(line, match, cpu_mhz_regex)) {
            frequency = std::stod(match[1]);
            break;
        }
    }
    
    // If we couldn't find the frequency, try an alternative method
    if (frequency == 0.0) {
        // Try to read from /sys/devices/system/cpu/cpu<core_id>/cpufreq/scaling_cur_freq
        std::stringstream path;
        path << "/sys/devices/system/cpu/cpu" << core_id << "/cpufreq/scaling_cur_freq";
        
        std::ifstream freqFile(path.str());
        if (freqFile.is_open()) {
            long freqKHz;
            freqFile >> freqKHz;
            frequency = freqKHz / 1000.0; // Convert from KHz to MHz
        }
    }
    
    return frequency;
}

std::vector<double> monitor_cpu_freq(int core_id, int duration_ms, int sampling_interval_ms) {
    std::vector<double> frequencies;
    int samples = duration_ms / sampling_interval_ms;
    
    for (int i = 0; i < samples; i++) {
        double freq = get_cpu_freq_mhz(core_id);
        frequencies.push_back(freq);
        std::this_thread::sleep_for(std::chrono::milliseconds(sampling_interval_ms));
    }
    
    return frequencies;
}

// A safer, alternative way to check for CPU features on Linux 
// by reading /proc/cpuinfo directly instead of executing CPUID
bool check_cpu_flag(const std::string& flag) {
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line;
    
    while (std::getline(cpuinfo, line)) {
        if (line.find("flags") != std::string::npos) {
            return line.find(flag) != std::string::npos;
        }
    }
    return false;
}

// CPU feature detection using alternative methods
bool has_sse() {
    return check_cpu_flag("sse");
}

bool has_sse2() {
    return check_cpu_flag("sse2");
}

bool has_avx() {
    return check_cpu_flag("avx");
}

bool has_avx2() {
    return check_cpu_flag("avx2");
}

bool has_avx512f() {
    return check_cpu_flag("avx512f");
}

bool has_amx() {
    return check_cpu_flag("amx_bf16") || check_cpu_flag("amx_tile");
}

// Collect frequencies from all available cores
std::map<int, double> get_all_core_frequencies() {
    std::map<int, double> all_frequencies;
    int core_count = get_core_count();
    
    for (int core_id = 0; core_id < core_count; core_id++) {
        all_frequencies[core_id] = get_cpu_freq_mhz(core_id);
    }
    
    return all_frequencies;
}

// Monitor frequencies of all cores over time
std::map<int, std::vector<double>> monitor_all_cpu_freq(int duration_ms, int sampling_interval_ms) {
    std::map<int, std::vector<double>> all_frequencies;
    int core_count = get_core_count();
    int samples = duration_ms / sampling_interval_ms;
    
    for (int i = 0; i < samples; i++) {
        for (int core_id = 0; core_id < core_count; core_id++) {
            double freq = get_cpu_freq_mhz(core_id);
            all_frequencies[core_id].push_back(freq);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sampling_interval_ms));
    }
    
    return all_frequencies;
}

// Run a function on a specific core
void run_on_core(int core_id, const std::function<void()>& func) {
    std::thread t([core_id, &func]() {
        pin_to_core(core_id);
        func();
    });
    
    t.join();
}

// Run a function on all cores in parallel
void run_on_all_cores(const std::function<void()>& func) {
    int core_count = get_core_count();
    std::vector<std::thread> threads;
    
    for (int core_id = 0; core_id < core_count; core_id++) {
        threads.emplace_back([core_id, &func]() {
            pin_to_core(core_id);
            func();
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
}

// Run a function on all cores sequentially
void run_on_all_cores_sequential(const std::function<void(int)>& func) {
    int core_count = get_core_count();
    
    for (int core_id = 0; core_id < core_count; core_id++) {
        run_on_core(core_id, [core_id, &func]() {
            func(core_id);
        });
    }
}

void print_all_core_frequencies() {
    std::map<int, double> frequencies = get_all_core_frequencies();
    
    std::cout << "CPU Frequencies for All Cores:" << std::endl;
    for (const auto& [core_id, freq] : frequencies) {
        std::cout << "  Core " << core_id << ": " << freq << " MHz" << std::endl;
    }
}

void print_cpu_info() {
    // Get CPU name
    std::string cpu_name = "Unknown";
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line;
    
    while (std::getline(cpuinfo, line)) {
        if (line.find("model name") != std::string::npos) {
            cpu_name = line.substr(line.find(":") + 1);
            // Trim leading whitespace
            cpu_name.erase(0, cpu_name.find_first_not_of(" \t"));
            break;
        }
    }
    
    std::cout << "CPU Information:" << std::endl;
    std::cout << "  Model: " << cpu_name << std::endl;
    std::cout << "  Cores: " << get_core_count() << std::endl;
    std::cout << "  Instruction Set Support:" << std::endl;
    std::cout << "    SSE:     " << (has_sse() ? "Yes" : "No") << std::endl;
    std::cout << "    SSE2:    " << (has_sse2() ? "Yes" : "No") << std::endl;
    std::cout << "    AVX:     " << (has_avx() ? "Yes" : "No") << std::endl;
    std::cout << "    AVX2:    " << (has_avx2() ? "Yes" : "No") << std::endl;
    std::cout << "    AVX512F: " << (has_avx512f() ? "Yes" : "No") << std::endl;
    std::cout << "    AMX:     " << (has_amx() ? "Yes" : "No") << std::endl;
    
    // Print frequencies of all cores
    std::cout << "\n  Core Frequencies:" << std::endl;
    std::map<int, double> frequencies = get_all_core_frequencies();
    for (const auto& [core_id, freq] : frequencies) {
        std::cout << "    Core " << core_id << ": " << freq << " MHz" << std::endl;
    }
}

void print_single_core_info(int core_id) {
    // Get CPU name
    std::string cpu_name = "Unknown";
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line;
    
    while (std::getline(cpuinfo, line)) {
        if (line.find("model name") != std::string::npos) {
            cpu_name = line.substr(line.find(":") + 1);
            // Trim leading whitespace
            cpu_name.erase(0, cpu_name.find_first_not_of(" \t"));
            break;
        }
    }
    
    std::cout << "CPU Information:" << std::endl;
    std::cout << "  Model: " << cpu_name << std::endl;
    std::cout << "  Cores: " << get_core_count() << std::endl;
    std::cout << "  Instruction Set Support:" << std::endl;
    std::cout << "    SSE:     " << (has_sse() ? "Yes" : "No") << std::endl;
    std::cout << "    SSE2:    " << (has_sse2() ? "Yes" : "No") << std::endl;
    std::cout << "    AVX:     " << (has_avx() ? "Yes" : "No") << std::endl;
    std::cout << "    AVX2:    " << (has_avx2() ? "Yes" : "No") << std::endl;
    std::cout << "    AVX512F: " << (has_avx512f() ? "Yes" : "No") << std::endl;
    std::cout << "    AMX:     " << (has_amx() ? "Yes" : "No") << std::endl;
    
    // Print frequency only for the selected core
    double freq = get_cpu_freq_mhz(core_id);
    std::cout << "\n  Core " << core_id << " Frequency: " << freq << " MHz" << std::endl;
}