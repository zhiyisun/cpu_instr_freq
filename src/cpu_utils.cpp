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
    
    // Print frequency of core 0 as an example
    std::cout << "  Current frequency of core 0: " << get_cpu_freq_mhz(0) << " MHz" << std::endl;
}