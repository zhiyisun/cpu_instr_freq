#include "cpu_utils.h"
#include "avx_benchmark.h"

#include <iostream>
#include <string>
#include <cstdlib>
#include <vector>
#include <thread>

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --help             Show this help message" << std::endl;
    std::cout << "  --instr=TYPE       Instruction set type (avx128, avx256, avx512, amx)" << std::endl;
    std::cout << "  --time=SECONDS     Duration of the benchmark in seconds (default: 5)" << std::endl;
    std::cout << "  --core=ID          CPU core to run the benchmark on (default: 0)" << std::endl;
    std::cout << "  --list             List available CPU features and exit" << std::endl;
    std::cout << std::endl;
    std::cout << "Example: " << program_name << " --instr=avx256 --time=10 --core=3" << std::endl;
}

int main(int argc, char** argv) {
    // Default parameters
    std::string instr_type = "avx256";
    int duration_sec = 5;
    int core_id = 0;
    bool show_help = false;
    bool list_features = false;
    
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
        } else if (arg == "--list") {
            list_features = true;
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
    
    // Display system information
    print_cpu_info();
    
    // Run the benchmark
    run_benchmark(instr_set, duration_sec, core_id);
    
    return 0;
}