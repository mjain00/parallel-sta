import matplotlib.pyplot as plt
import numpy as np





# Data: thread counts and corresponding times (in microseconds)
threads = [2, 4, 6, 8]
build_dag_times = [33240, 32121, 33177, 32344]
top_sort_times   = [1107205, 651059, 501770, 438067]
analyze_times    = [48606, 47691, 48090, 47809]

# Baseline (1-thread) times for speedup calculation
baseline_topo    = 1702872
baseline_analyze = 47038

# Compute speedup
topo_speedup    = baseline_topo / np.array(top_sort_times)
analyze_speedup = baseline_analyze / np.array(analyze_times)

# --- Speedup Plot ---
fig, ax = plt.subplots(figsize=(8, 5))
ax.plot(threads, topo_speedup,    marker='o', linestyle='--', linewidth=2,
        label='Forward Pass', color='#ff7f0e')
ax.plot(threads, analyze_speedup, marker='o', linestyle='--', linewidth=2,
        label='Backward Pass',    color='#2ca02c')

ax.set_title('Speedup vs Number of Threads')
ax.set_xlabel('Threads')
ax.set_ylabel('Speedup × (relative to 1 thread)')
ax.grid(True, linestyle='--', alpha=0.5)
ax.legend(fontsize=10)                  # normal legend call
ax.set_xticks(threads)
ax.set_ylim(0, max(topo_speedup)*1.1)

plt.tight_layout()

# --- DAG Build Time Plot ---
fig2, ax2 = plt.subplots(figsize=(8, 5))
bars = ax2.bar(threads, build_dag_times, color='#1f77b4', edgecolor='black')

ax2.set_title('Task Graph Build Time vs Number of Threads')
ax2.set_xlabel('Threads')
ax2.set_ylabel('Build Time (µs)')
ax2.grid(axis='y', linestyle='--', alpha=0.5)
ax2.set_xticks(threads)

# Label bars
for bar in bars:
    h = bar.get_height()
    ax2.text(bar.get_x() + bar.get_width()/2, h + 500, f'{int(h)}',
             ha='center', va='bottom', fontsize=9)

plt.tight_layout()

# --- Combined Time Plot (Stacked Bar Chart) ---
fig3, ax3 = plt.subplots(figsize=(8, 5))

# Create stacked bar chart for each thread
ax3.bar(threads, build_dag_times, label='Build TaskGraph', color='#1f77b4', edgecolor='black')
ax3.bar(threads, top_sort_times, bottom=build_dag_times, label='Forward Pass', color='#ff7f0e', edgecolor='black')
ax3.bar(threads, analyze_times, bottom=np.array(build_dag_times) + np.array(top_sort_times), label='Backward Pass', color='#2ca02c', edgecolor='black')

# Set titles and labels
ax3.set_title('Total Execution Time (Stacked Components) vs Number of Threads')
ax3.set_xlabel('Threads')
ax3.set_ylabel('Total Time (µs)')
ax3.grid(axis='y', linestyle='--', alpha=0.5)
ax3.legend(fontsize=10)

plt.tight_layout()

plt.show()
