#!/usr/bin/env python3
"""
CPU Frequency Monitor

This script monitors the CPU frequency of all cores at 1-second intervals
and outputs the data to a CSV file with timestamps.
"""

import os
import time
import csv
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

def main():
    core_count = get_core_count()
    print(f"Detected {core_count} CPU cores")
    
    # Prepare CSV file
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    csv_filename = f"cpu_frequencies_{timestamp}.csv"
    
    print(f"Recording CPU frequencies to {csv_filename}")
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

if __name__ == "__main__":
    main()