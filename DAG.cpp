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
            }
        }
    }
}

std::vector<int> DAG::topologicalSort(const ASIC& asic, const std::map<int, Cell>& cell_map) {
    std::unordered_map<int, int> inDegree;
    // std::unordered_map<int, int> delay;  // Store delay of each node
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
            arrival_time[node.first] = 0;
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
                  << " with current accumulated delay = " << arrival_time[current] << "\n";
        if (adjList.count(current) != 0) {
            for (int neighbor : adjList.at(current)) {
                inDegree[neighbor]--;
                int oldDelay = arrival_time[neighbor];
                int cellDelay = 0;
                
                // Check if the neighbor exists in the cell_map
                if (cell_map.count(neighbor)) {
                    // If the node exists in cell_map, accumulate the delay
                    cellDelay = cell_map.at(neighbor).delay;  // Assuming 'delay' is a property of the Cell
                }

                // Update the delay based on the current node's delay + the cell delay (if any)
                arrival_time[neighbor] = std::max(arrival_time[neighbor], arrival_time[current] + cellDelay);
    
                std::cout << "  -> Visiting neighbor " << neighbor
                          << ", decremented in-degree to " << inDegree[neighbor] << "\n";
                std::cout << "     Delay update: max(" << oldDelay << ", " 
                          << arrival_time[current] << " + " << cellDelay << ") = " << arrival_time[neighbor] << "\n";
    
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
        std::cout << "Node " << node << " | Accumulated delay: " << arrival_time[node] << "\n";
    }

    return result;
}

void DAG::reverseList() {
    for (const auto& [u, neighbors] : adjList) {
        for (int v : neighbors) {
            reverseAdjList[v].push_back(u);
        }
    }
}

void DAG::analyzeTiming(const ASIC& asic, const std::map<int, Cell>& cell_map, std::vector<int> &sorted) {
    std::unordered_map<int, int> required_time;
    std::unordered_map<int, float> slack;

    reverseList();

    std::cout << "\n=== Step 1: Initializing required times at outputs ===\n";
    for (int output : asic.outputs) {
        required_time[output] = CLOCK_PERIOD - SETUP_TIME;
        std::string name = asic.net_dict.count(output) ? asic.net_dict.at(output) : "Unknown";
        std::cout << "Output net " << name << " (ID: " << output << ") → Required time = "
                  << required_time[output] << "\n";
    }

    std::cout << "\n=== Step 2: Propagating required times (backward) ===\n";
    for (auto it = sorted.rbegin(); it != sorted.rend(); ++it) {
        int current = *it;

        if (required_time.find(current) == required_time.end()) {
            required_time[current] = INT64_MAX;
        }

        if (reverseAdjList.count(current)) {
            for (int fanin : reverseAdjList[current]) {
                float cell_delay = 0.0f;

                // delay from cell driving `current`
                if (cell_map.count(current)) {
                    cell_delay = cell_map.at(current).delay;
                }

                int candidate_time = required_time[current] - cell_delay;

                if (required_time.find(fanin) == required_time.end()) {
                    required_time[fanin] = candidate_time;
                } else {
                    required_time[fanin] = std::min(required_time[fanin], candidate_time);
                }

                std::string fanin_name = asic.net_dict.count(fanin) ? asic.net_dict.at(fanin) : "Unknown";
                std::cout << "  Fanin " << fanin_name << " (ID: " << fanin 
                          << ") → Required time updated to " << required_time[fanin] 
                          << " (via " << cell_delay << " delay)\n";
            }
        }
    }

    std::cout << "\n=== Step 3: Slack Computation ===\n";
    for (int net : sorted) {
        float at = arrival_time.count(net) ? arrival_time.at(net) : 0.0f;
        float rt = required_time.count(net) ? required_time[net] : CLOCK_PERIOD;

        float s = rt - at;
        slack[net] = s;

        std::string net_name = asic.net_dict.count(net) ? asic.net_dict.at(net) : "Unknown";

        std::cout << "Net " << net_name << " (ID: " << net << ")"
                  << " | Arrival: " << at
                  << " | Required: " << rt
                  << " | Slack: " << s;

        if (s < 0)
            std::cout << " VIOLATION!";
        else if (s == 0)
            std::cout << " CRITICAL PATH";

        std::cout << std::endl;
    }

}