#!/bin/bash

# run_benchmarks.sh - Script to automate CPU instruction frequency benchmarks
# Created: April 14, 2025

# Default settings
BINARY="./build/cpu_instr_freq"
DURATION=10  # Default benchmark duration in seconds
OUTPUT_DIR="./benchmark_results"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")

# Colors for better readability
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Create output directory if it doesn't exist
mkdir -p "${OUTPUT_DIR}"

# Function to display script usage
usage() {
  echo -e "${BLUE}Usage: $0 [options]${NC}"
  echo "Options:"
  echo "  -h, --help            Display this help message"
  echo "  -d, --duration N      Set benchmark duration to N seconds (default: 3)"
  echo "  -i, --instr TYPE      Run only specified instruction set"
  echo "                        TYPE: avx128, avx256, avx512, amx, basic_add"
  echo "  -c, --core N          Run only on specified core N (default: all tests)"
  echo "  -o, --output DIR      Save results to DIR (default: ./benchmark_results)"
  echo "  -s, --single-core     Run only single-core tests"
  echo "  -p, --parallel        Run only parallel tests"
  echo "  -q, --sequential      Run only sequential tests"
  echo "  -a, --all             Run all tests (default)"
  echo "Examples:"
  echo "  $0 --duration 5              # Run all tests with 5 second duration"
  echo "  $0 --instr avx256 --core 2   # Run only AVX256 on core 2"
  echo "  $0 --parallel --duration 3   # Run only parallel tests for 3 seconds"
}

# Parse command line options
RUN_SINGLE=true
RUN_PARALLEL=true
RUN_SEQUENTIAL=false
SPECIFIC_CORE=""
SPECIFIC_INSTR=""

while [[ $# -gt 0 ]]; do
  case $1 in
    -h|--help)
      usage
      exit 0
      ;;
    -d|--duration)
      DURATION="$2"
      shift 2
      ;;
    -i|--instr)
      SPECIFIC_INSTR="$2"
      shift 2
      ;;
    -c|--core)
      SPECIFIC_CORE="$2"
      shift 2
      ;;
    -o|--output)
      OUTPUT_DIR="$2"
      shift 2
      ;;
    -s|--single-core)
      RUN_SINGLE=true
      RUN_PARALLEL=false
      RUN_SEQUENTIAL=false
      shift
      ;;
    -p|--parallel)
      RUN_SINGLE=false
      RUN_PARALLEL=true
      RUN_SEQUENTIAL=false
      shift
      ;;
    -q|--sequential)
      RUN_SINGLE=false
      RUN_PARALLEL=false
      RUN_SEQUENTIAL=true
      shift
      ;;
    -a|--all)
      RUN_SINGLE=true
      RUN_PARALLEL=true
      RUN_SEQUENTIAL=false
      shift
      ;;
    *)
      echo -e "${RED}Error: Unknown option $1${NC}"
      usage
      exit 1
      ;;
  esac
done

# Validate binary exists
if [ ! -f "$BINARY" ]; then
  echo -e "${RED}Error: Benchmark binary not found at $BINARY${NC}"
  echo "Have you built the project? Run 'cd build && cmake .. && make' first."
  exit 1
fi

# Create results directory for this run
RESULTS_DIR="${OUTPUT_DIR}/${TIMESTAMP}"
mkdir -p "${RESULTS_DIR}"
echo -e "${GREEN}Saving benchmark results to: ${RESULTS_DIR}${NC}"

# Log file for this session
LOG_FILE="${RESULTS_DIR}/benchmark_summary.log"

# Create log header
echo "CPU Instruction Frequency Benchmark Results" > "${LOG_FILE}"
echo "Date: $(date)" >> "${LOG_FILE}"
echo "Duration: ${DURATION} seconds per test" >> "${LOG_FILE}"
echo "----------------------------------------" >> "${LOG_FILE}"

# Function to run benchmark
run_benchmark() {
  local instr=$1
  local args=$2
  local desc=$3
  local outfile="${RESULTS_DIR}/${instr}_${args// /_}.txt"
  
  echo -e "${YELLOW}Running: ${instr} ${desc}${NC}"
  echo "Command: ${BINARY} --instr=${instr} --time=${DURATION} ${args}" >> "${LOG_FILE}"
  echo "Running ${instr} ${desc}..." >> "${LOG_FILE}"
  
  # Run benchmark and capture output
  ${BINARY} --instr=${instr} --time=${DURATION} ${args} > "${outfile}" 2>&1
  
  # Check if benchmark ran successfully
  if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Completed: ${instr} ${desc}${NC}"
    echo "Status: Success" >> "${LOG_FILE}"
  else
    echo -e "${RED}✗ Failed: ${instr} ${desc}${NC}"
    echo "Status: Failed" >> "${LOG_FILE}"
  fi
  
  echo "" >> "${LOG_FILE}"
}

# List of instruction sets to benchmark
declare -a INSTRUCTIONS
if [ -n "$SPECIFIC_INSTR" ]; then
  INSTRUCTIONS=("$SPECIFIC_INSTR")
else
  INSTRUCTIONS=("avx128" "avx256" "avx512" "amx" "basic_add")
fi

# Run benchmarks
for instr in "${INSTRUCTIONS[@]}"; do
  # Single core tests
  if [ "$RUN_SINGLE" = true ]; then
    if [ -n "$SPECIFIC_CORE" ]; then
      # Run on specific core only
      run_benchmark "${instr}" "--core=${SPECIFIC_CORE}" "on core ${SPECIFIC_CORE}"
    else
      # Run on cores 0, 1, 2, and a higher core number as samples
      run_benchmark "${instr}" "--core=0" "on core 0"
      run_benchmark "${instr}" "--core=1" "on core 1"
      run_benchmark "${instr}" "--core=2" "on core 2"
      # Run on a higher core number (assuming 8 cores minimum)
      run_benchmark "${instr}" "--core=7" "on core 7"
    fi
  fi
  
  # Parallel all-cores test
  if [ "$RUN_PARALLEL" = true ] && [ -z "$SPECIFIC_CORE" ]; then
    run_benchmark "${instr}" "--all-cores" "on all cores in parallel"
  fi
  
  # Sequential all-cores test
  if [ "$RUN_SEQUENTIAL" = true ] && [ -z "$SPECIFIC_CORE" ]; then
    # For sequential test, use a shorter duration as it will run for N*cores time
    # Use 1 second to avoid very long test runs
    run_benchmark "${instr}" "--all-cores-seq --time=1" "on all cores sequentially (1s)"
  fi
done

echo -e "${GREEN}All benchmarks completed! Results saved to: ${RESULTS_DIR}${NC}"
echo "Summary log: ${LOG_FILE}"

# Display recommended next steps
echo -e "${BLUE}To examine results:${NC}"
echo "  • View the summary log: cat ${LOG_FILE}"
echo "  • Compare individual test results in ${RESULTS_DIR}"
echo "  • Analyze frequency variations between different instruction sets and cores"
