import os
import pandas as pd
import matplotlib.pyplot as plt

# Ensure output directory exists
os.makedirs('graphs', exist_ok=True)

# Read CSVs
acc = pd.read_csv('Iris_accuracy5JUL.csv')
nodes = pd.read_csv('Iris_nodes5JUL.csv')
actual_depth = pd.read_csv('Iris_actualDepth5JUL.csv')

# Clean up the data (remove empty rows)
acc = acc.dropna(how='all')
nodes = nodes.dropna(how='all')
actual_depth = actual_depth.dropna(how='all')

# Remove empty columns if any
acc = acc.loc[:, ~acc.columns.str.contains('^Unnamed')]
nodes = nodes.loc[:, ~nodes.columns.str.contains('^Unnamed')]
actual_depth = actual_depth.loc[:, ~actual_depth.columns.str.contains('^Unnamed')]

# Prepare x-axis (depths)
depth_labels = acc['Depth'].astype(str).tolist()
x = list(range(len(depth_labels)))

# Plot Accuracy vs Depth
plt.figure(figsize=(8,6))
for method in ['IG', 'IGR', 'NWIG']:
    plt.plot(x, acc[method], marker='o', label=method)
plt.xticks(x, depth_labels)
plt.xlabel('Max Tree Depth')
plt.ylabel('Average Accuracy (%)')
plt.title('Iris: Average Accuracy vs Max Tree Depth')
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig('graphs/Iris_accuracy_vs_depth.png')
plt.close()

# Plot Nodes vs Depth
plt.figure(figsize=(8,6))
for method in ['IG', 'IGR', 'NWIG']:
    plt.plot(x, nodes[method], marker='o', label=method)
plt.xticks(x, depth_labels)
plt.xlabel('Max Tree Depth')
plt.ylabel('Number of Nodes')
plt.title('Iris: Number of Nodes vs Max Tree Depth')
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig('graphs/Iris_nodes_vs_depth.png')
plt.close()

# Plot Actual Depth vs Max Depth
plt.figure(figsize=(8,6))
for method in ['IG', 'IGR', 'NWIG']:
    plt.plot(x, actual_depth[method], marker='o', label=method)
plt.xticks(x, depth_labels)
plt.xlabel('Max Tree Depth')
plt.ylabel('Actual Tree Depth')
plt.title('Iris: Actual Tree Depth vs Max Tree Depth')
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig('graphs/Iris_actualDepth_vs_maxDepth.png')
plt.close()

print("Graphs saved in the 'graphs/' folder.")