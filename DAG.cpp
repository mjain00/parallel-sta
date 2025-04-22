#include "DAG.hpp"
#include <iostream>
#include <chrono>

// Adds a directed edge from 'from' to 'to'
void DAG::addEdge(int from, int to) {
    adjList[from].push_back(to);
}


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


void DAG::removeCycles() {
    std::unordered_set<int> visited;
    std::unordered_set<int> recStack;

    std::function<void(int)> dfs = [&](int node) {
        visited.insert(node);
        recStack.insert(node);

        auto& neighbors = adjList[node]; // direct reference to the neighbor set
        for (auto it = neighbors.begin(); it != neighbors.end(); ) {
            int neighbor = *it;

            if (recStack.count(neighbor)) {
                // Found a back edge → remove it
                std::cout << "Removing back edge: " << node << " -> " << neighbor << "\n";
                it = neighbors.erase(it); // erase and continue
            } else if (!visited.count(neighbor)) {
                dfs(neighbor);
                ++it;
            } else {
                ++it;
            }
        }

        recStack.erase(node);
    };

    for (const auto& [node, _] : adjList) {
        if (!visited.count(node)) {
            dfs(node);
        }
    }
}

std::vector<int> DAG::topologicalSort(const ASIC& asic, const std::map<int, Cell>& cell_map) {
    std::unordered_map<int, int> inDegree;
    std::vector<int> result;
    std::queue<int> q;

    if (verbose) std::cout << "\n=== Step 1: Calculating in-degrees ===\n";

    for (const auto& node : adjList) {
        inDegree[node.first]; // defaults to 0 if not present
        for (int neighbor : node.second) {
            inDegree[neighbor]++;
            if (verbose) {
                std::cout << "Edge: " << node.first << " -> " << neighbor 
                          << " | Incrementing in-degree to " << inDegree[neighbor] << "\n";
            }
        }
    }

    if (verbose) std::cout << "\n=== Step 2: Enqueuing in-degree 0 nodes ===\n";

    for (const auto& [node, degree] : inDegree) {
        if (degree == 0) {
            q.push(node);
            arrival_time[node] = 0;
            if (verbose) {
                std::cout << "Enqueued node " << node << " | delay = 0\n";
            }
        }
    }

    if (verbose) std::cout << "\n=== Step 3: Processing queue ===\n";

    while (!q.empty()) {
        int current = q.front(); q.pop();
        result.push_back(current);

        if (verbose) {
            std::cout << "\nProcessing node " << current
                      << " | accumulated delay = " << arrival_time[current] << "\n";
        }

        for (int neighbor : adjList[current]) {
            inDegree[neighbor]--;
            updateArrivalTime(current, neighbor, cell_map);

            if (inDegree[neighbor] == 0) {
                q.push(neighbor);
                if (verbose) {
                    std::cout << "     Enqueued " << neighbor << " (in-degree now 0)\n";
                }
            }
        }
    }

    if (result.size() != inDegree.size()) {
        std::cerr << "\nError: Graph has a cycle!\n";
        return {};
    }

    if (verbose) {
        std::cout << "\n=== Final Topological Order and Delays ===\n";
        for (int node : result) {
            std::cout << "Node " << node << " | Delay: " << arrival_time[node] << "\n";
        }
    }

    return result;
}


// Delay update function
void DAG::updateArrivalTime(int current, int neighbor, const std::map<int, Cell>& cell_map) {
    int oldDelay = arrival_time[neighbor];
    int cellDelay = 0;

    if (cell_map.count(neighbor)) {
        cellDelay = cell_map.at(neighbor).delay;
    }

    arrival_time[neighbor] = std::max(arrival_time[neighbor], arrival_time[current] + cellDelay);

    if (verbose) {
        std::cout << "  -> Visiting neighbor " << neighbor
                  << ", delay update: max(" << oldDelay << ", "
                  << arrival_time[current] << " + " << cellDelay
                  << ") = " << arrival_time[neighbor] << "\n";
    }
}


void DAG::reverseList() {
    for (const auto& [u, neighbors] : adjList) {
        for (int v : neighbors) {
            reverseAdjList[v].push_back(u);
        }
    }
}

std::unordered_map<int, float> DAG::analyzeTiming(const ASIC& asic, const std::map<int, Cell>& cell_map, std::vector<int> &sorted) {
    std::unordered_map<int, int> required_time;
    std::unordered_map<int, float> slack;

    reverseList();

    if (verbose) {
        std::cout << "\n=== Step 1: Initializing required times at outputs ===\n";
    }
    
    for (int output : asic.outputs) {
        required_time[output] = CLOCK_PERIOD - SETUP_TIME;
        std::string name = asic.net_dict.count(output) ? asic.net_dict.at(output) : "Unknown";

        if (verbose) {
            std::cout << "Output net " << name << " (ID: " << output << ") → Required time = "
                  << required_time[output] << "\n";
        }        
    }

    if (verbose) {
        std::cout << "\n=== Step 2: Propagating required times (backward) ===\n";
    }
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

                if (verbose) {
                    std::cout << "  Fanin " << fanin_name << " (ID: " << fanin 
                            << ") → Required time updated to " << required_time[fanin] 
                            << " (via " << cell_delay << " delay)\n";
                }
            }
        }
    }

    if (verbose) {
        std::cout << "\n=== Step 3: Slack Computation ===\n";
    }
    
    for (int net : sorted) {
        float at = arrival_time.count(net) ? arrival_time.at(net) : 0.0f;
        float rt = required_time.count(net) ? required_time[net] : CLOCK_PERIOD;

        float s = rt - at;
        slack[net] = s;

        std::string net_name = asic.net_dict.count(net) ? asic.net_dict.at(net) : "Unknown";

        if (verbose) {
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
    return slack;
}