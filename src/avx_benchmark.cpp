#include "avx_benchmark.h"
#include "cpu_utils.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <algorithm>
#include <numeric>
#include <string>
#include <cstring>
#include <sstream>
#include <mutex>
#include <iomanip>

// Global variable to control benchmark execution
std::atomic<bool> g_running(false);

// Mutex for thread-safe console output
std::mutex g_console_mutex;

// Get the string name of the instruction set
std::string get_instruction_set_name(InstructionSet instr_set) {
    switch(instr_set) {
        case InstructionSet::AVX128:
            return "AVX128/SSE";
        case InstructionSet::AVX256:
            return "AVX256";
        case InstructionSet::AVX512:
            return "AVX512";
        case InstructionSet::AMX:
            return "AMX";
        case InstructionSet::BASIC_ADD:
            return "Basic ADD";
        default:
            return "Unknown";
    }
}

// Convert string to instruction set enum
InstructionSet string_to_instruction_set(const std::string& str) {
    std::string lower_str = str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(), ::tolower);
    
    if (lower_str == "avx128" || lower_str == "sse" || lower_str == "128") {
        return InstructionSet::AVX128;
    } else if (lower_str == "avx256" || lower_str == "avx2" || lower_str == "256") {
        return InstructionSet::AVX256;
    } else if (lower_str == "avx512" || lower_str == "512") {
        return InstructionSet::AVX512;
    } else if (lower_str == "amx") {
        return InstructionSet::AMX;
    } else if (lower_str == "basic_add" || lower_str == "add" || lower_str == "basic") {
        return InstructionSet::BASIC_ADD;
    } else {
        std::cerr << "Unknown instruction set: " << str << std::endl;
        std::cerr << "Available options: avx128, avx256, avx512, amx, basic_add" << std::endl;
        exit(1);
    }
}

// SSE benchmark function (safe for most x86 CPUs, using SSE instructions)
extern "C" void benchmark_sse(size_t iterations) {
    asm volatile(
        "movq %0, %%rcx\n"     // Move iterations to rcx register
        
        // Initialize xmm registers with data
        "movq $0x3f800000, %%rax\n"    // 1.0f in IEEE-754
        "movd %%eax, %%xmm0\n"
        "pshufd $0, %%xmm0, %%xmm0\n"  // Replicate to all elements
        
        "movq $0x40000000, %%rax\n"    // 2.0f in IEEE-754
        "movd %%eax, %%xmm1\n"
        "pshufd $0, %%xmm1, %%xmm1\n"  // Replicate to all elements
        
        "1:\n"                 // Label for loop
        
        // SSE instructions (128-bit)
        "movaps %%xmm0, %%xmm2\n"
        "addps %%xmm1, %%xmm2\n"
        "mulps %%xmm1, %%xmm2\n"
        "movaps %%xmm2, %%xmm3\n"
        "addps %%xmm0, %%xmm3\n"
        "mulps %%xmm1, %%xmm3\n"
        "movaps %%xmm3, %%xmm4\n"
        "addps %%xmm0, %%xmm4\n"
        "mulps %%xmm4, %%xmm0\n"
        
        "decq %%rcx\n"         // Decrement counter
        "jnz 1b\n"             // Jump back to label 1 if rcx != 0
        
        : // No outputs
        : "r"(iterations)      // Input constraint: iterations in a register
        : "rax", "rcx", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4" // Clobbered registers
    );
}

// AVX-128/SSE benchmark function
extern "C" void benchmark_avx128(size_t iterations) {
    // First try AVX if available
    if (has_avx()) {
        asm volatile(
            "movq %0, %%rcx\n"     // Move iterations to rcx register
            
            // Initialize xmm registers with data
            "vxorps %%xmm0, %%xmm0, %%xmm0\n"
            "vxorps %%xmm1, %%xmm1, %%xmm1\n"
            "vaddps %%xmm0, %%xmm1, %%xmm0\n"
            
            "1:\n"                 // Label for loop
            
            // AVX-128 instructions
            "vmovaps %%xmm0, %%xmm1\n"
            "vaddps %%xmm1, %%xmm0, %%xmm0\n"
            "vmulps %%xmm1, %%xmm0, %%xmm0\n"
            "vshufps $0x1B, %%xmm0, %%xmm0, %%xmm1\n"
            "vaddps %%xmm1, %%xmm0, %%xmm0\n"
            "vmovaps %%xmm0, %%xmm2\n"
            "vmovaps %%xmm0, %%xmm3\n"
            "vaddps %%xmm2, %%xmm3, %%xmm3\n"
            "vmovaps %%xmm3, %%xmm4\n"
            "vmovaps %%xmm4, %%xmm5\n"
            "vaddps %%xmm4, %%xmm5, %%xmm5\n"
            "vmulps %%xmm3, %%xmm5, %%xmm5\n"
            "vaddps %%xmm5, %%xmm0, %%xmm0\n"
            
            "decq %%rcx\n"         // Decrement counter
            "jnz 1b\n"             // Jump back to label 1 if rcx != 0
            
            : // No outputs
            : "r"(iterations)      // Input constraint: iterations in a register
            : "rcx", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5" // Clobbered registers
        );
    }
    // Fallback to SSE if AVX is not available
    else if (has_sse2()) {
        benchmark_sse(iterations);
    }
}

// AVX-256 benchmark function
extern "C" void benchmark_avx256(size_t iterations) {
    asm volatile(
        "movq %0, %%rcx\n"     // Move iterations to rcx register
        
        // Initialize ymm registers with data
        "vxorps %%ymm0, %%ymm0, %%ymm0\n"
        "vxorps %%ymm1, %%ymm1, %%ymm1\n"
        
        "1:\n"                 // Label for loop
        
        // AVX2 instructions (256-bit)
        "vmovaps %%ymm0, %%ymm1\n"
        "vaddps %%ymm1, %%ymm0, %%ymm0\n"
        "vmulps %%ymm1, %%ymm0, %%ymm0\n"
        "vpermpd $0x1B, %%ymm0, %%ymm2\n"
        "vaddps %%ymm2, %%ymm0, %%ymm0\n"
        "vmovaps %%ymm0, %%ymm3\n"
        "vaddps %%ymm3, %%ymm0, %%ymm0\n"
        "vmulps %%ymm3, %%ymm0, %%ymm0\n"
        "vaddps %%ymm1, %%ymm0, %%ymm0\n"
        
        "decq %%rcx\n"         // Decrement counter
        "jnz 1b\n"             // Jump back to label 1 if rcx != 0
        
        "vzeroupper\n"         // Zero upper bits of YMM registers
        
        : // No outputs
        : "r"(iterations)      // Input constraint: iterations in a register
        : "rcx", "ymm0", "ymm1", "ymm2", "ymm3" // Clobbered registers
    );
}

// AVX-512 benchmark function
extern "C" void benchmark_avx512(size_t iterations) {
    asm volatile(
        "movq %0, %%rcx\n"     // Move iterations to rcx register
        
        // Initialize zmm registers with data
        "vpxorq %%zmm0, %%zmm0, %%zmm0\n"
        "vpxorq %%zmm1, %%zmm1, %%zmm1\n"
        
        "1:\n"                 // Label for loop
        
        // AVX-512 instructions (512-bit)
        "vmovaps %%zmm0, %%zmm1\n"
        "vaddps %%zmm1, %%zmm0, %%zmm0\n"
        "vmulps %%zmm1, %%zmm0, %%zmm0\n"
        "vaddps %%zmm1, %%zmm0, %%zmm2\n"
        "vfmadd132ps %%zmm0, %%zmm1, %%zmm2\n"
        "vmovaps %%zmm2, %%zmm3\n"
        "vfmadd213ps %%zmm0, %%zmm1, %%zmm3\n"
        "vaddps %%zmm3, %%zmm0, %%zmm0\n"
        "vmulps %%zmm3, %%zmm0, %%zmm0\n"
        
        "decq %%rcx\n"         // Decrement counter
        "jnz 1b\n"             // Jump back to label 1 if rcx != 0
        
        "vzeroupper\n"         // Zero upper bits of ZMM registers
        
        : // No outputs
        : "r"(iterations)      // Input constraint: iterations in a register
        : "rcx", "zmm0", "zmm1", "zmm2", "zmm3" // Clobbered registers
    );
}

// AMX benchmark function
extern "C" void benchmark_amx(size_t iterations) {
    // AMX requires specific setup with LDTILECFG instruction first
    // This is a placeholder that demonstrates AMX usage pattern
    // In a real implementation, we would use proper tile configuration and operations
    
    asm volatile(
        "movq %0, %%rcx\n"     // Move iterations to rcx register
        "1:\n"                 // Label for loop
        
        // AMX operation simulation with basic instructions
        // In reality, we would use tdpbf16ps, tdpbssd, tilezero, tileloadd, tilestored, etc.
        "xor %%rax, %%rax\n"
        "xor %%rbx, %%rbx\n"
        "xor %%rdx, %%rdx\n"
        "inc %%rax\n"
        "inc %%rbx\n"
        "inc %%rdx\n"
        
        "decq %%rcx\n"         // Decrement counter
        "jnz 1b\n"             // Jump back to label 1 if rcx != 0
        
        : // No outputs
        : "r"(iterations)      // Input constraint: iterations in a register
        : "rax", "rbx", "rcx", "rdx" // Clobbered registers
    );
}

// Basic integer ADD benchmark function
extern "C" void benchmark_basic_add(size_t iterations) {
    asm volatile(
        "movq %0, %%rcx\n"     // Move iterations to rcx register
        
        // Initialize registers with data
        "movq $1, %%rax\n"
        "movq $2, %%rbx\n"
        
        "1:\n"                 // Label for loop
        
        // Basic integer add instructions
        "addq %%rbx, %%rax\n"
        "addq %%rax, %%rbx\n"
        "addq %%rbx, %%rax\n"
        "addq %%rax, %%rbx\n"
        "addq %%rbx, %%rax\n"
        "addq %%rax, %%rbx\n"
        "addq %%rbx, %%rax\n"
        "addq %%rax, %%rbx\n"
        "addq %%rbx, %%rax\n"
        "addq %%rax, %%rbx\n"
        
        "decq %%rcx\n"         // Decrement counter
        "jnz 1b\n"             // Jump back to label 1 if rcx != 0
        
        : // No outputs
        : "r"(iterations)      // Input constraint: iterations in a register
        : "rax", "rbx", "rcx"  // Clobbered registers
    );
}

// Thread function to run benchmarks
void benchmark_thread_func(InstructionSet instr_set, size_t iterations) {
    // Run appropriate benchmark based on instruction set
    switch(instr_set) {
        case InstructionSet::AVX128:
            benchmark_avx128(iterations);
            break;
        case InstructionSet::AVX256:
            benchmark_avx256(iterations);
            break;
        case InstructionSet::AVX512:
            benchmark_avx512(iterations);
            break;
        case InstructionSet::AMX:
            benchmark_amx(iterations);
            break;
        case InstructionSet::BASIC_ADD:
            benchmark_basic_add(iterations);
            break;
    }
}

// Thread function to monitor CPU frequency
void monitor_thread_func(int core_id, std::vector<double>& frequencies) {
    const int sampling_interval_ms = 100; // Sample every 100ms
    
    while (g_running) {
        double freq = get_cpu_freq_mhz(core_id);
        frequencies.push_back(freq);
        std::this_thread::sleep_for(std::chrono::milliseconds(sampling_interval_ms));
    }
}

// Print detailed benchmark results
void print_benchmark_result(const BenchmarkResult& result, const std::string& instr_name) {
    std::lock_guard<std::mutex> lock(g_console_mutex);
    
    std::cout << "\nBenchmark Results for Core " << result.core_id << ":" << std::endl;
    std::cout << "  Instruction Set: " << instr_name << std::endl;
    std::cout << "  Frequency Statistics:" << std::endl;
    std::cout << "    Minimum: " << std::fixed << std::setprecision(2) << result.min_freq << " MHz" << std::endl;
    std::cout << "    Maximum: " << std::fixed << std::setprecision(2) << result.max_freq << " MHz" << std::endl;
    std::cout << "    Average: " << std::fixed << std::setprecision(2) << result.avg_freq << " MHz" << std::endl;
    
    // Print frequency timeline only if verbose output is needed
    /*
    // Print a subset of frequency samples
    const int max_samples_to_show = 10;
    std::cout << "\n  Frequency Timeline (samples):" << std::endl;
    if (result.frequencies.size() <= max_samples_to_show) {
        // Show all samples
        for (size_t i = 0; i < result.frequencies.size(); i++) {
            std::cout << "    " << (i * 100) << "ms: " << result.frequencies[i] << " MHz" << std::endl;
        }
    } else {
        // Show a subset of samples
        size_t step = result.frequencies.size() / max_samples_to_show;
        for (size_t i = 0; i < result.frequencies.size(); i += step) {
            std::cout << "    " << (i * 100) << "ms: " << result.frequencies[i] << " MHz" << std::endl;
        }
        // Always show the last sample
        std::cout << "    " << ((result.frequencies.size() - 1) * 100) << "ms: " 
                  << result.frequencies[result.frequencies.size() - 1] << " MHz" << std::endl;
    }
    */
}

// Run the benchmark with specified instruction set and return results
BenchmarkResult run_benchmark_with_result(InstructionSet instr_set, int duration_sec, int core_id) {
    BenchmarkResult result;
    result.core_id = core_id;
    result.success = false;
    
    // Check if the CPU supports the requested instruction set
    bool supported = true;
    
    switch(instr_set) {
        case InstructionSet::AVX128:
            supported = has_sse2();  // Minimum requirement for AVX128 fallback
            break;
        case InstructionSet::AVX256:
            supported = has_avx2();
            break;
        case InstructionSet::AVX512:
            supported = has_avx512f();
            break;
        case InstructionSet::AMX:
            supported = has_amx();
            break;
        case InstructionSet::BASIC_ADD:
            supported = true; // Basic integer add is supported on all CPUs
            break;
    }
    
    if (!supported) {
        // Don't print anything here, just return the result indicating failure
        return result;
    }
    
    // Pin to specified core
    pin_to_core(core_id);
    
    // Start the benchmark thread
    g_running = true;
    result.frequencies.clear();
    
    // Create a monitoring thread
    std::thread monitor(monitor_thread_func, core_id, std::ref(result.frequencies));
    
    // Give monitor thread a chance to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    {
        std::lock_guard<std::mutex> lock(g_console_mutex);
        std::cout << "Running " << get_instruction_set_name(instr_set) 
                  << " benchmark on core " << core_id << "..." << std::endl;
    }
    
    // Start benchmark
    const size_t iterations_per_batch = 10000000; // 10 million iterations per batch
    auto start_time = std::chrono::steady_clock::now();
    auto end_time = start_time + std::chrono::seconds(duration_sec);
    
    while (std::chrono::steady_clock::now() < end_time) {
        benchmark_thread_func(instr_set, iterations_per_batch);
    }
    
    // Stop the monitor thread
    g_running = false;
    if (monitor.joinable()) {
        monitor.join();
    }
    
    // Calculate statistics
    if (result.frequencies.empty()) {
        return result;  // Return with success = false
    }
    
    result.min_freq = *std::min_element(result.frequencies.begin(), result.frequencies.end());
    result.max_freq = *std::max_element(result.frequencies.begin(), result.frequencies.end());
    result.avg_freq = std::accumulate(result.frequencies.begin(), result.frequencies.end(), 0.0) / result.frequencies.size();
    result.success = true;
    
    return result;
}

// Main benchmark runner function (for backward compatibility)
void run_benchmark(InstructionSet instr_set, int duration_sec, int core_id) {
    BenchmarkResult result = run_benchmark_with_result(instr_set, duration_sec, core_id);
    
    if (!result.success) {
        std::string instr_name = get_instruction_set_name(instr_set);
        
        std::lock_guard<std::mutex> lock(g_console_mutex);
        std::cerr << "The CPU does not support " << instr_name << " instructions." << std::endl;
        std::cerr << "Skipping this benchmark." << std::endl;
        return;
    }
    
    print_benchmark_result(result, get_instruction_set_name(instr_set));
    
    // Print all frequency measurements if requested (legacy detailed output)
    std::lock_guard<std::mutex> lock(g_console_mutex);
    std::cout << "\n  Frequency Timeline (100ms intervals):" << std::endl;
    const int max_samples_to_show = 50; // Limit the number of samples to show
    
    if (result.frequencies.size() <= max_samples_to_show) {
        // Show all samples
        for (size_t i = 0; i < result.frequencies.size(); i++) {
            std::cout << "    " << (i * 100) << "ms: " << result.frequencies[i] << " MHz" << std::endl;
        }
    } else {
        // Show a subset of samples
        size_t step = result.frequencies.size() / max_samples_to_show;
        for (size_t i = 0; i < result.frequencies.size(); i += step) {
            std::cout << "    " << (i * 100) << "ms: " << result.frequencies[i] << " MHz" << std::endl;
        }
        // Always show the last sample
        std::cout << "    " << ((result.frequencies.size() - 1) * 100) << "ms: " 
                  << result.frequencies[result.frequencies.size() - 1] << " MHz" << std::endl;
    }
}