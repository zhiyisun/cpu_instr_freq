#!/usr/bin/env python3
"""
CPU Frequency Monitor

This script monitors the CPU frequency of all cores or a specific core at 1-second intervals
and outputs the data to a CSV file with timestamps.

Usage:
  ./monitor_cpu_freq.py           # Monitor all CPU cores
  ./monitor_cpu_freq.py -c N      # Monitor only CPU core N (where N is a number)
  ./monitor_cpu_freq.py --core=N  # Monitor only CPU core N (where N is a number)
"""

import os
import time
import csv
import argparse
from datetime import datetime
from statistics import mean

def get_core_count():
    """Determine the number of CPU cores in the system."""
    return len([f for f in os.listdir('/sys/devices/system/cpu') if f.startswith('cpu') and f[3:].isdigit()])

def read_cpu_freq(core_id):
    """Read the current frequency of a specific CPU core in MHz."""
    try:
        freq_file = f"/sys/devices/system/cpu/cpu{core_id}/cpufreq/scaling_cur_freq"
        with open(freq_file, 'r') as f:
            # Convert from kHz to MHz
            return int(f.read().strip()) / 1000
    except (IOError, FileNotFoundError):
        return 0  # Return 0 if frequency can't be read

def parse_arguments():
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(description='Monitor CPU frequency of all cores or a specific core.')
    parser.add_argument('-c', '--core', type=int, help='CPU core ID to monitor (default: monitor all cores)')
    return parser.parse_args()

def monitor_all_cores(core_count):
    """Monitor all CPU cores and write data to CSV."""
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    csv_filename = f"cpu_frequencies_{timestamp}.csv"
    
    print(f"Recording frequencies for all {core_count} CPU cores to {csv_filename}")
    print("Press Ctrl+C to stop recording")
    
    try:
        with open(csv_filename, 'w', newline='') as csvfile:
            # Create column headers
            headers = ['Timestamp'] + [f'CPU{i}' for i in range(core_count)] + ['Average']
            writer = csv.writer(csvfile)
            writer.writerow(headers)
            
            while True:
                # Get timestamp
                timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                
                # Read frequencies for all cores
                frequencies = [read_cpu_freq(i) for i in range(core_count)]
                
                # Calculate average frequency
                avg_freq = mean(frequencies) if frequencies else 0
                
                # Write data to CSV
                row = [timestamp] + frequencies + [avg_freq]
                writer.writerow(row)
                csvfile.flush()  # Ensure data is written to disk immediately
                
                print(f"\r{timestamp} - Avg: {avg_freq:.2f} MHz", end='')
                
                # Wait 1 second before next reading
                time.sleep(1)
                
    except KeyboardInterrupt:
        print("\nStopping frequency monitoring")
        print(f"Data saved to {csv_filename}")

def monitor_single_core(core_id):
    """Monitor a specific CPU core and write data to CSV."""
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    csv_filename = f"cpu{core_id}_frequency_{timestamp}.csv"
    
    print(f"Recording frequency for CPU core {core_id} to {csv_filename}")
    print("Press Ctrl+C to stop recording")
    
    try:
        with open(csv_filename, 'w', newline='') as csvfile:
            # Create column headers
            headers = ['Timestamp', f'CPU{core_id}']
            writer = csv.writer(csvfile)
            writer.writerow(headers)
            
            while True:
                # Get timestamp
                timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                
                # Read frequency for the specified core
                frequency = read_cpu_freq(core_id)
                
                # Write data to CSV
                row = [timestamp, frequency]
                writer.writerow(row)
                csvfile.flush()  # Ensure data is written to disk immediately
                
                print(f"\r{timestamp} - CPU{core_id}: {frequency:.2f} MHz", end='')
                
                # Wait 1 second before next reading
                time.sleep(1)
                
    except KeyboardInterrupt:
        print("\nStopping frequency monitoring")
        print(f"Data saved to {csv_filename}")

def main():
    args = parse_arguments()
    core_count = get_core_count()
    print(f"Detected {core_count} CPU cores")
    
    if args.core is not None:
        if 0 <= args.core < core_count:
            monitor_single_core(args.core)
        else:
            print(f"Error: Invalid core ID {args.core}. Must be between 0 and {core_count-1}.")
            exit(1)
    else:
        monitor_all_cores(core_count)

if __name__ == "__main__":
    main()