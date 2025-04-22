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
    std::cout << "WE HAVE MADE IT HER" << std::endl;

    // Calculate in-degrees
    for (const auto& node : adjList) {
        inDegree[node.first]; // Defaults to 0 if not present
        for (int neighbor : node.second) {
            inDegree[neighbor]++;
        }
    }

    // Enqueue nodes with 0 in-degree
    for (const auto& [node, degree] : inDegree) {
        if (degree == 0) {
            q.push(node);
            arrival_time[node] = 0;
        }
    }

    // Process nodes and calculate RC delay
    while (!q.empty()) {
        int current = q.front();
        q.pop();
        result.push_back(current);

        // For each outgoing connection from 'current', calculate RC delay
        for (int neighbor : adjList[current]) {
            // Check if current and neighbor are valid keys in the cell_map
            if (cell_map.find(current) != cell_map.end() && cell_map.find(neighbor) != cell_map.end()) {
                double rc_delay = computeRCDelay(cell_map.at(current), cell_map.at(neighbor));
                double slew_rate = computeSlewRate(cell_map.at(current), cell_map.at(neighbor));
                delays_and_slews.push_back({current, neighbor, rc_delay, slew_rate});
                updateArrivalTime(current, neighbor, cell_map);

                // You can store this RC delay or use it to update other metrics
            } else {
                std::cerr << "These are signals - don't correspond to components" << std::endl;
                
            }

            // Decrease in-degree and enqueue if all dependencies are processed
            inDegree[neighbor]--;
            if (inDegree[neighbor] == 0) {
                q.push(neighbor);
            }
        }
    }

    return result;
}


void DAG::updateArrivalTime(int current, int neighbor, const std::map<int, Cell>& cell_map) {
    // Find the corresponding delay and slew between current -> neighbor
    for (const auto& [from, to, rc_delay, slew] : delays_and_slews) {
        if (from == current && to == neighbor) {
            double total_delay = rc_delay + slew;

            double old_arrival = arrival_time[neighbor];
            double new_arrival = arrival_time[current] + total_delay;

            arrival_time[neighbor] = std::max(old_arrival, new_arrival);

            std::cout << "Updating arrival time for cell " << neighbor
                      << ": max(" << old_arrival << ", "
                      << arrival_time[current] << " + " << total_delay
                      << ") = " << arrival_time[neighbor] << std::endl;
            return;
        }
    }

    std::cerr << "Warning: No delay/slew entry found for edge " << current << " -> " << neighbor << std::endl;
}


double DAG::computeSlewRate(const Cell& current_cell, const Cell& neighbor_cell) {
    // Assuming voltage swing (V) is a constant value, e.g., 1V (you can adjust this value)
    double voltage_swing = 1.0; // V

    // Calculate slew rate based on the RC time constant
    double rc_time_constant = current_cell.resistance * neighbor_cell.capacitance;
    double slew_rate = voltage_swing / rc_time_constant;
    double slew_time = voltage_swing / slew_rate; // (V / (V/s)) = seconds

    // Print Slew Rate
    std::cout << "Computing Slew Rate: "
              << "Resistance of current cell = " << current_cell.resistance
              << ", Capacitance of neighbor cell = " << neighbor_cell.capacitance
              << " => Slew Rate = " << slew_rate << " V/s" << std::endl;

    return slew_time;
}

// Function to compute RC delay between two cells
double DAG:: computeRCDelay(const Cell& current_cell, const Cell& neighbor_cell) {
    // Calculate RC delay based on the resistance and capacitance of both cells
    double rc_delay = current_cell.resistance * neighbor_cell.capacitance;

    // Print RC delay
    std::cout << "Computing RC Delay: "
              << "Resistance of current cell = " << current_cell.id
              << ", Capacitance of neighbor cell = " << neighbor_cell.capacitance
              << " => RC Delay = " << rc_delay << std::endl;

    return rc_delay;
}


// std::vector<int> DAG::topologicalSort(const ASIC& asic, const std::map<int, Cell>& cell_map) {
//     std::unordered_map<int, int> inDegree;
//     std::vector<int> result;
//     std::queue<int> q;

//     if (verbose) std::cout << "\n=== Step 1: Calculating in-degrees ===\n";

//     for (const auto& node : adjList) {
//         inDegree[node.first]; // defaults to 0 if not present
//         for (int neighbor : node.second) {
//             inDegree[neighbor]++;
//             if (verbose) {
//                 std::cout << "Edge: " << node.first << " -> " << neighbor 
//                           << " | Incrementing in-degree to " << inDegree[neighbor] << "\n";
//             }
//         }
//     }

//     if (verbose) std::cout << "\n=== Step 2: Enqueuing in-degree 0 nodes ===\n";

//     for (const auto& [node, degree] : inDegree) {
//         if (degree == 0) {
//             q.push(node);
//             arrival_time[node] = 0;
//             if (verbose) {
//                 std::cout << "Enqueued node " << node << " | delay = 0\n";
//             }
//         }
//     }

//     if (verbose) std::cout << "\n=== Step 3: Processing queue ===\n";

//     while (!q.empty()) {
//         int current = q.front(); q.pop();
//         result.push_back(current);

//         if (verbose) {
//             std::cout << "\nProcessing node " << current
//                       << " | accumulated delay = " << arrival_time[current] << "\n";
//         }

//         for (int neighbor : adjList[current]) {
//             inDegree[neighbor]--;
//             updateArrivalTime(current, neighbor, cell_map);

//             if (inDegree[neighbor] == 0) {
//                 q.push(neighbor);
//                 if (verbose) {
//                     std::cout << "     Enqueued " << neighbor << " (in-degree now 0)\n";
//                 }
//             }
//         }
//     }

//     if (result.size() != inDegree.size()) {
//         std::cerr << "\nError: Graph has a cycle!\n";
//         return {};
//     }

//     if (verbose) {
//         std::cout << "\n=== Final Topological Order and Delays ===\n";
//         for (int node : result) {
//             std::cout << "Node " << node << " | Delay: " << arrival_time[node] << "\n";
//         }
//     }

//     return result;
// }


// // Delay update function
// void DAG::updateArrivalTime(int current, int neighbor, const std::map<int, Cell>& cell_map) {
//     int oldDelay = arrival_time[neighbor];
//     int cellDelay = 0;

//     if (cell_map.count(neighbor)) {
//         cellDelay = cell_map.at(neighbor).delay;
//     }

//     arrival_time[neighbor] = std::max(arrival_time[neighbor], arrival_time[current] + cellDelay);

//     if (verbose) {
//         std::cout << "  -> Visiting neighbor " << neighbor
//                   << ", delay update: max(" << oldDelay << ", "
//                   << arrival_time[current] << " + " << cellDelay
//                   << ") = " << arrival_time[neighbor] << "\n";
//     }
// }
























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