import matplotlib.pyplot as plt

# Threads
threads = [1, 2, 4, 8, 16]

# Cache miss data
cache_misses = {
    'Node Parallelism': [5898962, 5975676, 5975676, 5821847, 6013944],
    'Task Loop': [6038338, 5919131, 6218882, 5991306, 6047741],
    'Task Graph': [7867450, 7654468, 7659363, 8075325, 7561056]
}

# Plot
plt.figure(figsize=(10, 6))

for algo, misses in cache_misses.items():
    plt.plot(threads, misses, marker='o', label=algo)

# Settings
plt.xlabel('Threads')
plt.ylabel('Cache Misses')
plt.title('Cache Misses Comparison for Bigcircuit')
plt.grid(True)
plt.legend()
plt.xticks(threads)
plt.tight_layout()

# Save and show
plt.savefig('bigcircuit_cache_misses_comparison.png')
plt.show()

