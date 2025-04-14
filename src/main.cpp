#include "cpu_utils.h"
#include "avx_benchmark.h"

#include <iostream>
#include <string>
#include <cstdlib>
#include <vector>
#include <thread>
#include <map>
#include <iomanip> // Added for std::setw and std::setprecision

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --help             Show this help message" << std::endl;
    std::cout << "  --instr=TYPE       Instruction set type (avx128, avx256, avx512, amx)" << std::endl;
    std::cout << "  --time=SECONDS     Duration of the benchmark in seconds (default: 5)" << std::endl;
    std::cout << "  --core=ID          CPU core to run the benchmark on (default: 0)" << std::endl;
    std::cout << "  --all-cores        Run the benchmark on all cores in parallel" << std::endl;
    std::cout << "  --all-cores-seq    Run the benchmark on all cores sequentially" << std::endl;
    std::cout << "  --list             List available CPU features and exit" << std::endl;
    std::cout << "  --monitor-freq     Monitor CPU frequency during benchmark" << std::endl;
    std::cout << "  --freq-only        Only display frequencies of all cores and exit" << std::endl;
    std::cout << std::endl;
    std::cout << "Example: " << program_name << " --instr=avx256 --time=10 --core=3" << std::endl;
    std::cout << "Example: " << program_name << " --instr=avx256 --time=10 --all-cores" << std::endl;
}

void run_benchmark_with_frequency_monitoring(InstructionSet instr_set, int duration_sec, int core_id) {
    // Start frequency monitoring in a separate thread
    std::thread monitor_thread([core_id, duration_sec]() {
        // Sample every 100ms
        int sampling_interval_ms = 100;
        auto frequencies = monitor_cpu_freq(core_id, duration_sec * 1000, sampling_interval_ms);
        
        std::cout << "\nFrequency measurements for Core " << core_id << ":" << std::endl;
        double sum = 0.0;
        for (size_t i = 0; i < frequencies.size(); i++) {
            std::cout << "  " << (i * sampling_interval_ms / 1000.0) << "s: " << frequencies[i] << " MHz" << std::endl;
            sum += frequencies[i];
        }
        
        if (!frequencies.empty()) {
            std::cout << "Average frequency: " << (sum / frequencies.size()) << " MHz" << std::endl;
        }
    });
    
    // Run the benchmark
    run_benchmark(instr_set, duration_sec, core_id);
    
    // Wait for monitoring to complete
    monitor_thread.join();
}

void run_benchmark_on_all_cores(InstructionSet instr_set, int duration_sec, bool monitor_freq) {
    std::cout << "Running benchmark on all cores in parallel..." << std::endl;
    
    // Start frequency monitoring if requested
    std::map<int, std::vector<double>> all_frequencies;
    std::thread monitor_thread;
    
    if (monitor_freq) {
        monitor_thread = std::thread([duration_sec, &all_frequencies]() {
            // Sample every 100ms
            int sampling_interval_ms = 100;
            all_frequencies = monitor_all_cpu_freq(duration_sec * 1000, sampling_interval_ms);
        });
    }
    
    // Collect results from each core
    int core_count = get_core_count();
    std::vector<BenchmarkResult> results(core_count);
    std::vector<std::thread> threads;
    
    // Launch benchmark threads for each core
    for (int core_id = 0; core_id < core_count; core_id++) {
        threads.emplace_back([core_id, instr_set, duration_sec, &results]() {
            results[core_id] = run_benchmark_with_result(instr_set, duration_sec, core_id);
        });
    }
    
    // Wait for all benchmarks to complete
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    
    // Wait for monitoring to complete if requested
    if (monitor_freq && monitor_thread.joinable()) {
        monitor_thread.join();
    }
    
    // Display results in an organized manner, one core at a time
    std::string instr_name = get_instruction_set_name(instr_set);
    std::cout << "\n========== Benchmark Results for " << instr_name << " ==========\n" << std::endl;
    
    // Print summary table
    std::cout << "Core ID  |   Min Freq (MHz)  |   Max Freq (MHz)  |   Avg Freq (MHz)" << std::endl;
    std::cout << "---------|-------------------|-------------------|------------------" << std::endl;
    
    for (int core_id = 0; core_id < core_count; core_id++) {
        if (results[core_id].success) {
            std::cout << std::setw(8) << core_id << " | " 
                      << std::fixed << std::setw(17) << std::setprecision(2) << results[core_id].min_freq << " | "
                      << std::fixed << std::setw(17) << std::setprecision(2) << results[core_id].max_freq << " | "
                      << std::fixed << std::setw(17) << std::setprecision(2) << results[core_id].avg_freq << std::endl;
        } else {
            std::cout << std::setw(8) << core_id << " |         N/A        |         N/A        |         N/A" << std::endl;
        }
    }
    
    // If monitoring was done separately, show those results too
    if (monitor_freq && !all_frequencies.empty()) {
        std::cout << "\nFrequency Monitoring Results:" << std::endl;
        for (const auto& [core_id, frequencies] : all_frequencies) {
            double sum = 0.0;
            for (const auto& freq : frequencies) {
                sum += freq;
            }
            double avg = frequencies.empty() ? 0.0 : sum / frequencies.size();
            std::cout << "  Core " << core_id << " average: " << std::fixed << std::setprecision(2) << avg << " MHz" << std::endl;
        }
    }
}

void run_benchmark_on_all_cores_sequential(InstructionSet instr_set, int duration_sec, bool monitor_freq) {
    std::cout << "Running benchmark on all cores sequentially..." << std::endl;
    
    // Collect results from each core one at a time
    int core_count = get_core_count();
    std::vector<BenchmarkResult> results(core_count);
    
    for (int core_id = 0; core_id < core_count; core_id++) {
        std::cout << "Running benchmark on core " << core_id << "..." << std::endl;
        results[core_id] = run_benchmark_with_result(instr_set, duration_sec, core_id);
    }
    
    // Display results in an organized manner
    std::string instr_name = get_instruction_set_name(instr_set);
    std::cout << "\n========== Sequential Benchmark Results for " << instr_name << " ==========\n" << std::endl;
    
    // Print summary table
    std::cout << "Core ID  |   Min Freq (MHz)  |   Max Freq (MHz)  |   Avg Freq (MHz)" << std::endl;
    std::cout << "---------|-------------------|-------------------|------------------" << std::endl;
    
    for (int core_id = 0; core_id < core_count; core_id++) {
        if (results[core_id].success) {
            std::cout << std::setw(8) << core_id << " | " 
                      << std::fixed << std::setw(17) << std::setprecision(2) << results[core_id].min_freq << " | "
                      << std::fixed << std::setw(17) << std::setprecision(2) << results[core_id].max_freq << " | "
                      << std::fixed << std::setw(17) << std::setprecision(2) << results[core_id].avg_freq << std::endl;
        } else {
            std::cout << std::setw(8) << core_id << " |         N/A        |         N/A        |         N/A" << std::endl;
        }
    }
}

int main(int argc, char** argv) {
    // Default parameters
    std::string instr_type = "avx256";
    int duration_sec = 5;
    int core_id = 0;
    bool show_help = false;
    bool list_features = false;
    bool use_all_cores = false;
    bool use_all_cores_sequential = false;
    bool monitor_freq = false;
    bool freq_only = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            show_help = true;
        } else if (arg.find("--instr=") == 0) {
            instr_type = arg.substr(8);
        } else if (arg.find("--time=") == 0) {
            duration_sec = std::atoi(arg.substr(7).c_str());
        } else if (arg.find("--core=") == 0) {
            core_id = std::atoi(arg.substr(7).c_str());
        } else if (arg == "--all-cores") {
            use_all_cores = true;
        } else if (arg == "--all-cores-seq") {
            use_all_cores_sequential = true;
        } else if (arg == "--list") {
            list_features = true;
        } else if (arg == "--monitor-freq") {
            monitor_freq = true;
        } else if (arg == "--freq-only") {
            freq_only = true;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            print_usage(argv[0]);
            return 1;
        }
    }
    
    if (show_help) {
        print_usage(argv[0]);
        return 0;
    }
    
    // Only print CPU info and return if --list was specified
    if (list_features) {
        print_cpu_info();
        return 0;
    }
    
    // Only print CPU frequencies and return if --freq-only was specified
    if (freq_only) {
        print_all_core_frequencies();
        return 0;
    }
    
    // Validate parameters
    if (duration_sec <= 0) {
        std::cerr << "Error: Duration must be greater than 0" << std::endl;
        return 1;
    }
    
    int max_core = get_max_core_id();
    if (core_id < 0 || core_id > max_core) {
        std::cerr << "Error: Core ID must be between 0 and " << max_core << std::endl;
        return 1;
    }
    
    // Convert instruction type string to enum
    InstructionSet instr_set;
    try {
        instr_set = string_to_instruction_set(instr_type);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    // Display system information based on the benchmark mode
    if (use_all_cores || use_all_cores_sequential) {
        // For all-cores modes, show all CPU info
        print_cpu_info();
    } else {
        // For single-core mode, only show info for the selected core
        print_single_core_info(core_id);
    }
    
    // Run the benchmark based on the chosen options
    if (use_all_cores) {
        run_benchmark_on_all_cores(instr_set, duration_sec, monitor_freq);
    } else if (use_all_cores_sequential) {
        run_benchmark_on_all_cores_sequential(instr_set, duration_sec, monitor_freq);
    } else if (monitor_freq) {
        run_benchmark_with_frequency_monitoring(instr_set, duration_sec, core_id);
    } else {
        // Run the benchmark on a single core
        run_benchmark(instr_set, duration_sec, core_id);
    }
    
    return 0;
}