#include "DAG.hpp"
#include <iostream>

// Adds a directed edge from 'from' to 'to'
void DAG::addEdge(int from, int to) {
    adjList[from].push_back(to);
}

// Displays the graph in a readable format
// void DAG::displayGraph() {
//     for (const auto& node : adjList) {
//         std::cout << "Node " << node.first << " has edges to: ";
//         for (const auto& neighbor : node.second) {
//             std::cout << neighbor << " ";
//         }
//         std::cout << std::endl;
//     }
// }


void DAG::displayGraph(const ASIC& asic) {
    for (const auto& node : adjList) {
        // Get the net name from net_dict using the node's ID
        std::string node_name = (asic.net_dict.find(node.first) != asic.net_dict.end()) 
                                ? asic.net_dict.at(node.first)
                                : "Unknown";

        std::cout << "Node " << node_name << " (ID: " << node.first << ") has edges to: ";

        for (const auto& neighbor : node.second) {
            // Get the neighbor's net name from net_dict
            std::string neighbor_name = (asic.net_dict.find(neighbor) != asic.net_dict.end()) 
                                        ? asic.net_dict.at(neighbor)
                                        : "Unknown";
            std::cout << neighbor_name << " (ID: " << neighbor << ") ";
        }

        std::cout << std::endl;
    }
}

// Builds the DAG from the ASIC structure (connecting inputs and outputs of cells)
void DAG::buildFromASIC(const ASIC& asic) {
    for (const auto& cell : asic.cells) {
        for (const auto& input : cell.inputs) {
            for (const auto& output : cell.outputs) {
                addEdge(input, output);  // Create edges based on the cell's inputs and outputs
                netToCell[output] = cell.id;
            }
        }
    }
}



std::vector<int> DAG::topologicalSort(const ASIC& asic) {
    std::unordered_map<int, int> inDegree;
    std::unordered_map<int, int> delay;  // Store delay of each node
    std::vector<int> result;
    std::queue<int> q;

    std::cout << "\n=== Step 1: Calculating in-degrees ===\n";
    for (const auto& node : adjList) {
        if (inDegree.find(node.first) == inDegree.end()) {
            inDegree[node.first] = 0;
        }
        for (int neighbor : node.second) {
            inDegree[neighbor]++;
            std::cout << "Edge: " << node.first << " -> " << neighbor 
                      << " | Incrementing in-degree of " << neighbor 
                      << " to " << inDegree[neighbor] << "\n";
        }
    }

    std::cout << "\n=== Step 2: Enqueuing in-degree 0 nodes ===\n";
    for (const auto& node : inDegree) {
        if (node.second == 0) {
            q.push(node.first);
            delay[node.first] = 0;
            std::cout << "Enqueued node " << node.first 
                      << " with in-degree 0, initial delay = 0\n";
        }
    }

    std::cout << "\n=== Step 3: Processing queue ===\n";
    while (!q.empty()) {
        int current = q.front();
        q.pop();
        result.push_back(current);

        std::cout << "\nProcessing node " << current 
                  << " with current accumulated delay = " << delay[current] << "\n";
        if (adjList.count(current) != 0) {
            for (int neighbor : adjList.at(current)) {
                inDegree[neighbor]--;
                int oldDelay = delay[neighbor];
                int cellDelay = 0;
                if (netToCell.count(neighbor)) {
                    int cellId = netToCell[neighbor];
                    cellDelay = asic.cells[cellId].delay;
                }
                delay[neighbor] = std::max(delay[neighbor], delay[current] + cellDelay);
    
                std::cout << "  -> Visiting neighbor " << neighbor
                          << ", decremented in-degree to " << inDegree[neighbor] << "\n";
                std::cout << "     Delay update: max(" << oldDelay << ", " 
                          << delay[current] << " + 1) = " << delay[neighbor] << "\n";
    
                if (inDegree[neighbor] == 0) {
                    q.push(neighbor);
                    std::cout << "     Enqueued " << neighbor << " (now in-degree 0)\n";
                }
            }
        } else {
            std::cout << "  -> No neighbors for node " << current << "\n";
        }
    }

    if (result.size() != inDegree.size()) {
        std::cerr << "\nError: Graph has a cycle!\n";
        return {};
    }

    std::cout << "\n=== Final Topological Order and Delays ===\n";
    for (int node : result) {
        std::cout << "Node " << node << " | Accumulated delay: " << delay[node] << "\n";
    }

    return result;
}
