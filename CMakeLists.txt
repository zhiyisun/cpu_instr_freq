cmake_minimum_required(VERSION 3.10)
project(cpu_instr_freq)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable assembly support
enable_language(ASM)

# Add executable
add_executable(cpu_instr_freq
  src/main.cpp
  src/cpu_utils.cpp
  src/avx_benchmark.cpp
)

# Include directories
target_include_directories(cpu_instr_freq PRIVATE include)

# Set compiler flags - using the most conservative set
# Remove all advanced instruction set flags to prevent issues
target_compile_options(cpu_instr_freq PRIVATE
  -Wall
  -Wextra
  -msse
  -msse2
)

# Add more aggressive flags only when explicitly compiling AVX code
set_source_files_properties(
  src/avx_benchmark.cpp
  PROPERTIES COMPILE_FLAGS "-mavx -mavx2 -mfma"
)

# Link pthread
target_link_libraries(cpu_instr_freq PRIVATE pthread)