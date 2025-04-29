import matplotlib.pyplot as plt

# Threads
threads = [2, 4, 8, 16]

# New timing data

timings = {
    'simple': [12, 13, 16, 21, 210],
    'adder': [52, 51, 55, 78, 162],
    'longpath': [40, 91, 48, 58, 134],
    'balanced': [38, 41, 43, 50, 120],
    'mult': [185, 227, 200, 238, 244],
    'bigadder': [329, 353, 414, 368, 420],
    'bigcircuit': [13229, 11846, 12224, 12756, 13649]
} #backward prop for task loop

timings = {
    'simple': [21, 85, 138, 296, 513],
    'adder': [96, 139, 185, 333, 827],
    'longpath': [118, 125, 168, 309, 617],
    'balanced': [111, 127, 200, 289, 917],
    'mult': [386, 408, 439, 611, 1529],
    'bigadder': [611, 662, 642, 868, 1582],
    'bigcircuit': [66043, 39650, 28079, 29582, 31611]
} #fwd prop for task loop


# Create plot
plt.figure(figsize=(12, 7))

for circuit, times in timings.items():
    time1 = times[0]  # Time at 1 thread
    speedups = [time1 / t for t in times[1:]]  # Speedup = T(1) / T(n)
    plt.plot(threads, speedups, marker='o', label=circuit)

# Plot settings
plt.xlabel('Threads')
plt.ylabel('Speedup')
plt.title('Speedup vs Threads per Circuit - Forward Propagation')
plt.grid(True)
plt.legend()
plt.xticks(threads)
plt.tight_layout()

# Save and show
plt.savefig('speedup_vs_threads_latest.png')
plt.show()
