import matplotlib.pyplot as plt

# Data
circuit_names = ['simple', 'adder', 'longpath', 'balanced', 'mult', 'bigadder', 'bigcircuit']
times = [58, 83, 62, 349, 516, 843, 618275]

# Create the plot
plt.figure(figsize=(10, 6))
plt.plot(circuit_names, times, marker='o', linestyle='-')
plt.xlabel('Circuit Name')
plt.ylabel('Time (microseconds)')
plt.title('Forward Pass Timing per Circuit')
plt.grid(True)

# Rotate x-axis labels if needed
plt.xticks(rotation=45)

# Show plot
plt.tight_layout()
plt.savefig("fwd_seq.png")
plt.show()
