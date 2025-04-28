import matplotlib.pyplot as plt
import numpy as np

# Data
labels = ['Parsing JSON', 'Creating Cell Map', 'Building DAG', 
          'Topological Sort (Forward Pass)', 'Analyze Timing (Backward Pass)']
times = [3855730, 7595, 9244, 2558349, 47772]

# Create figure and axis
fig, ax = plt.subplots(figsize=(10, 6))

# Create the bar chart
bars = ax.bar(labels, times, color='lightgray', edgecolor='black', linewidth=1.2)

# Add title and labels with LaTeX-like font
ax.set_title('Execution Times for Different Stages', fontsize=16, fontweight='bold', family='serif')
ax.set_xlabel('Stage', fontsize=14, family='serif')
ax.set_ylabel('Time (us)', fontsize=14, family='serif')

# Customize gridlines with dashed lines
ax.yaxis.grid(True, linestyle='--', alpha=0.6)

# Add the bar values on top of the bars
for bar in bars:
    yval = bar.get_height()
    ax.text(bar.get_x() + bar.get_width()/2, yval + 0.05 * max(times), 
            round(yval, 2), ha='center', va='bottom', fontsize=12, fontweight='bold', family='serif')

# Rotate the x-axis labels for better readability
plt.xticks(rotation=45, ha='right', fontsize=12, family='serif')

# Adjust layout to ensure the labels are not cut off
plt.tight_layout()

# Show the plot
plt.show()
