import matplotlib.pyplot as plt
import numpy as np

# Threads
threads = [2, 4, 8, 16]

# Timing data (replace these 1-thread numbers with your actual 1-thread times)
timings = {
    'simple': [58, 103, 164, 264, 1265],
    'adder': [83, 132, 195, 348, 1476],
    'longpath': [62, 112, 167, 298, 1415],
    'balanced': [349, 118, 202, 311, 1085],
    'mult': [516, 446, 394, 545, 2039],
    'bigadder': [843, 673, 577, 687, 2624],
    'bigcircuit': [618275, 332688, 187431, 120565, 141769]
}

# Create plot
plt.figure(figsize=(12, 7))

for circuit, times in timings.items():
    time1 = times[0]  # Time at 1 thread
    speedups = [time1 / t for t in times[1:]]  # Speedup = T(1) / T(n)
    plt.plot(threads, speedups, marker='o', label=circuit)

# Plot settings
plt.xlabel('Threads')
plt.ylabel('Speedup')
plt.title('Speedup vs Threads per Circuit')
plt.grid(True)
plt.legend()
plt.xticks(threads)
plt.tight_layout()

# Save and show
plt.savefig('speedup_vs_threads.png')
plt.show()
