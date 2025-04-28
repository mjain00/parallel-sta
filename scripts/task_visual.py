import matplotlib.pyplot as plt
import networkx as nx

# Build the DAG
G = nx.DiGraph()

# Define tasks
nodes = [
    "N1_slew", "N1_rc", "N1_arrival",
    "N2_slew", "N2_rc", "N2_arrival",
    "N3_slew", "N3_rc", "N3_arrival"
]
G.add_nodes_from(nodes)

# Intra-node edges (slew → rc → arrival)
intra = [
    ("N1_slew",    "N1_rc"),    ("N1_rc",    "N1_arrival"),
    ("N2_slew",    "N2_rc"),    ("N2_rc",    "N2_arrival"),
    ("N3_slew",    "N3_rc"),    ("N3_rc",    "N3_arrival"),
]
G.add_edges_from(intra)

# Converging edges: Node1_arrival & Node2_arrival → N3_slew
G.add_edge("N1_arrival", "N3_slew")
G.add_edge("N2_arrival", "N3_slew")

# Manual positions: columns at x=0,1,2 and rows y=2,1,0
pos = {
    "N1_slew":    (0, 2),
    "N1_rc":      (0, 1),
    "N1_arrival": (0, 0),
    "N2_slew":    (1, 2),
    "N2_rc":      (1, 1),
    "N2_arrival": (1, 0),
    "N3_slew":    (2, 2),
    "N3_rc":      (2, 1),
    "N3_arrival": (2, 0),
}

# Plot
plt.figure(figsize=(8, 4))

# Draw nodes
nx.draw_networkx_nodes(G, pos,
                       node_size=1200,
                       node_color="skyblue",
                       edgecolors="black")

# Draw labels on nodes
labels = {
    "N1_slew":    "Node1\nSlew",
    "N1_rc":      "Node1\nRC",
    "N1_arrival": "Node1\nArrival",
    "N2_slew":    "Node2\nSlew",
    "N2_rc":      "Node2\nRC",
    "N2_arrival": "Node2\nArrival",
    "N3_slew":    "Node3\nSlew",
    "N3_rc":      "Node3\nRC",
    "N3_arrival": "Node3\nArrival",
}
nx.draw_networkx_labels(G, pos, labels=labels, font_size=10, font_weight="bold")

# Draw directed edges with straight arrows
nx.draw_networkx_edges(G, pos,
                       edgelist=intra,
                       arrowstyle='-|>',
                       arrowsize=12,
                       edge_color='gray',
                       width=2)
nx.draw_networkx_edges(G, pos,
                       edgelist=[("N1_arrival","N3_slew"), ("N2_arrival","N3_slew")],
                       arrowstyle='-|>',
                       arrowsize=12,

                       
                       edge_color='gray',
                       width=2)

plt.title("Task Graph: Slew→RC→Arrival with Node1 & Node2 → Node3", fontsize=14, fontweight='bold')
plt.axis('off')
plt.tight_layout()
plt.show()
