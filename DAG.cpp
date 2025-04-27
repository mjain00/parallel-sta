#include "DAG.hpp"
#include <iostream>
#include <chrono>

// Adds a directed edge from 'from' to 'to'
void DAG::addEdge(int from, int to) {
    adjList[from].push_back(to);
}

void DAG::createTaskGraph()
{
    for (const auto& [node, neighbors] : adjList) {
        std::string rc = std::to_string(node) + "_rc";
        std::string slew = std::to_string(node) + "_slew";
        std::string arr = std::to_string(node) + "_arrival";
    
        taskGraph[rc].push_back(slew);
        taskGraph[slew].push_back(arr);
    
        for (int neighbor : neighbors) {
            std::string neighbor_rc = std::to_string(neighbor) + "_rc";
            taskGraph[arr].push_back(neighbor_rc);
        }
    }
    
    std::cout << "We are done with task graph \n";

    
}

void DAG::printTaskGraph() {
    std::cout << "Task Graph Dependencies (DAG):\n";
    for (const auto& [task, dependents] : taskGraph) {
        std::cout << task << " -> ";
        if (dependents.empty()) {
            std::cout << "{}";
        } else {
            for (const auto& dep : dependents) {
                std::cout << dep << " ";
            }
        }
        std::cout << "\n";
    }
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

    std::cout << "We are done with back edge: " << "\n";

}


std::vector<int> DAG::topologicalSort(const ASIC& asic, const std::map<int, Cell>& cell_map) {
    std::unordered_map<int, int> inDegree;
    std::vector<int> result;
    std::queue<int> q;

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
                double rc_delay = computeRCDelay(cell_map.at(current), cell_map.at(neighbor),current, neighbor);
                double slew_rate = computeSlewRate(cell_map.at(current), cell_map.at(neighbor),current, neighbor);
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



std::vector<int> DAG::topological_TaskGraph(DAG& dag, const std::map<int, Cell>& cell_map,const ASIC& asic) {
    std::map<std::string, int> inDegree;
    std::vector<int> result;
    std::vector<std::string> q;

    if (verbose) {
        std::cout << "\n=== Step 1: Calculating in-degrees (TaskGraph) ===\n";
    }

    for (const auto& [task, deps] : taskGraph) {
        if (!inDegree.count(task)) inDegree[task] = 0;
        for (const std::string& dep : deps) {
            if (!inDegree.count(dep)) inDegree[dep] = 0;
            inDegree[dep]++;
    
            if (verbose) {
                std::cout << "Edge: " << task << " -> " << dep 
                          << " | Incrementing in-degree of " << dep 
                          << " to " << inDegree[dep] << "\n";
            }
        }
    }

    if (verbose) {
        std::cout << "\n=== Step 2: Enqueuing in-degree 0 tasks ===\n";
    }

    for (const auto& [task, deg] : inDegree) {
        if (deg == 0) {
            q.push_back(task);
            if (verbose) {
                std::cout << "TID: " << omp_get_thread_num() 
                          << " | Enqueued initial task: " << task 
                          << " with in-degree = 0\n";
            }
        }
    }

    if (verbose) {
        std::cout << "\n=== Step 3: Processing task queue (parallel) ===\n";
    }

    while (!q.empty()) {
        std::vector<std::string> next_q;
        std::vector<std::string> q_copy = q;

        if (verbose) {
            std::cout << "\n-- New Iteration with " << q_copy.size() << " tasks in queue --\n";
            for (const auto& task : q_copy) {
                std::cout << "Task in queue: " << task << "\n";
            }
        }

#pragma omp parallel
        {
            // int thread_id = omp_get_thread_num();
            // std::cout << "Thread " << thread_id << " starting\n";
        
            std::vector<std::string> local_next;

#pragma omp for
            for (int i = 0; i < q_copy.size(); ++i) {
                std::string current = q_copy[i];

                size_t sep = current.find('_');
                int cell_id = std::stoi(current.substr(0, sep));
                string stage = (current.substr(sep+1));

                if(stage == "arrival"){
                    #pragma omp critical
                    {
                        result.push_back(cell_id);
                    }
                }
// #pragma omp critical
//                 {
//                     result.push_back(cell_id);
//                 }

                if (verbose) {
                    std::cout << "TID " << omp_get_thread_num() 
                              << " | Processing task: " << current 
                              << " | Cell ID: " << cell_id << "\n";
                }

                dag.processQueue(current, dag, cell_map,asic);

                for (const std::string& neighbor : taskGraph[current]) {
                    int new_in_degree;
#pragma omp atomic capture
                    new_in_degree = --inDegree[neighbor];

                    if (verbose) {
                        std::cout << "TID " << omp_get_thread_num() 
                                  << " | Processed edge: " << current << " -> " << neighbor 
                                  << " | New in-degree of " << neighbor << " = " << new_in_degree << "\n";
                    }

                    if (new_in_degree == 0) {
                        local_next.push_back(neighbor);
                        if (verbose) {
                            std::cout << "TID " << omp_get_thread_num() 
                                      << " | Enqueuing " << neighbor << " into local next queue\n";
                        }
                    }
                }
            }

#pragma omp critical
            {
                if (verbose) {
                    std::cout << "TID " << omp_get_thread_num() 
                              << " | Merging " << local_next.size() << " tasks into global next queue\n";
                }
                next_q.insert(next_q.end(), local_next.begin(), local_next.end());
            }

            local_next.clear();
            // std::cout << "Thread " << thread_id << " finished\n";

        }

        if (verbose) {
            std::cout << "-- End of iteration. Total tasks queued for next round: " << next_q.size() << "\n";
        }

        q = next_q;
        next_q.clear();
    }

    if (result.empty()) {
        std::cerr << "\nError: No tasks processed. Possible cycle in task graph.\n";
        return {};
    }

    if (verbose) {
        std::cout << "\n=== Final Task Order (Topologically Sorted) ===\n";
        for (int node : result) {
            std::cout << "Cell ID: " << node << "\n";
        }
    }

    return result;
}

void DAG::processQueue(const std::string& task, DAG& dag,const std::map<int, Cell>& cell_map,const ASIC& asic) {
    
    size_t sep = task.find('_');
    if (sep == std::string::npos) {
        std::cerr << "Malformed task: " << task << " (missing underscore '_')" << std::endl;
        return;
    }

    int cell_id = std::stoi(task.substr(0, sep));
    std::string stage = task.substr(sep + 1);

    if (stage == "rc") {
        for (int neighbor : adjList[cell_id]) {
            if (cell_map.find((cell_id))!= cell_map.end() && cell_map.find(neighbor) != cell_map.end()) {
                dag.computeRCDelay(cell_map.at(cell_id), cell_map.at(neighbor), cell_id, neighbor);
            }
        }
        if(verbose){
            std::cout <<"We are done processing for rc \n";
        }
    } 
    else if (stage == "slew") {
        for (int neighbor : adjList[cell_id]) {
            if (cell_map.find((cell_id))!= cell_map.end() && cell_map.find(neighbor) != cell_map.end()) {
                dag.computeSlewRate(cell_map.at(cell_id), cell_map.at(neighbor), cell_id, neighbor);
            }

        
        }
        if(verbose){
            std::cout <<"We are done processing for slew \n";
        }
    } 
    else if (stage == "arrival") {

        for (int neighbor : adjList[cell_id]) {
            if (cell_map.find((cell_id))!= cell_map.end() && cell_map.find(neighbor) != cell_map.end()) {
                dag.updateArrivalTime(cell_id, neighbor, cell_map);
            }
        

        }
        if(verbose){
            std::cout <<"We are done processing for arrival \n";
        }
    
    } 
    else if (stage == "be_required") {
        // For "be_required", we propagate the required state backwards to fan-ins
        for (int fanin : reverseAdjList[cell_id]) { // Using reverse adjacency list for backward pass
            // if (cell_map.find(cell_id) != cell_map.end() && cell_map.find(fanin) != cell_map.end()) {
                // You could add custom logic here to handle the required state propagation
                dag.propagateBeRequired(cell_map.at(cell_id), cell_id, fanin,asic);
                if(verbose){
                    std::cout <<"We are PROCESSING FOR TO BE REQUIRED \n";
                }
        
            // }
        }
    }
    else {
        std::cerr << "Unknown task type: " << task << std::endl;
    }
}

void DAG::propagateBeRequired(const Cell& current_cell, int current_id, int fanin_id, const ASIC& asic) {
    // Retrieve the current required time for the current cell (be_required)

    
    float required_time_for_current = required_time[current_id];
    
    // Calculate the required time for the fan-in cell by subtracting the delay of the current cell
    float required_time_for_fanin = required_time_for_current - current_cell.delay;

    // If the fan-in cell doesn't have a required time yet, set it to the calculated required time
    if (required_time.find(fanin_id) == required_time.end()) {
        required_time[fanin_id] = required_time_for_fanin;
    } else {
        // If the fan-in already has a required time, take the minimum between the current required time
        // and the newly calculated one (to account for the critical path)
        required_time[fanin_id] = std::min(required_time[fanin_id], required_time_for_fanin);
    }

    // Log the update (optional, depends on verbosity flag)
    std::string fanin_name = asic.net_dict.count(fanin_id) ? asic.net_dict.at(fanin_id) : "Unknown";
    if (verbose) {
        std::cout << "THIS IS NEW CODE: Fan-in " << fanin_name << " (ID: " << fanin_id
                  << ") → Required time updated to " << required_time[fanin_id]
                  << " (via " << current_cell.delay << " delay)\n";
    }
}

std::unordered_map<int, float> DAG::computeSlack(const ASIC& asic, const std::vector<int>& sorted) {
    std::unordered_map<int, float> slack;

    if (verbose) {
        std::cout << "\n=== Step 3: Slack Computation ===\n";
    }

    for (int net : sorted) {
        // Get the arrival time (or set to 0.0f if not found)
        float at = arrival_time.count(net) ? arrival_time.at(net) : 0.0f;
        
        // Get the required time (or set to CLOCK_PERIOD if not found)
        float rt = required_time.count(net) ? required_time[net] : CLOCK_PERIOD;
        
        // Compute the slack
        float s = rt - at;
        
        // Store the slack value
        slack[net] = s;

        // Get the net name (if available) for debugging purposes
        std::string net_name = asic.net_dict.count(net) ? asic.net_dict.at(net) : "Unknown";

        if (verbose) {
            // Print the details for the current net
            std::cout << "Net " << net_name << " (ID: " << net << ")"
                      << " | Arrival: " << at
                      << " | Required: " << rt
                      << " | Slack: " << s;
            
            // Check if there's a timing violation
            if (s < 0) {
                std::cout << " VIOLATION!";
            } else if (s == 0) {
                std::cout << " CRITICAL PATH";
            }

            std::cout << std::endl;
        }
    }

    return slack;
}


void DAG::initializeRequiredTime(const ASIC& asic,const std::map<int, Cell>& cell_map) {
    // Initialize required time for output cells
    for (int output : asic.outputs) {
        required_time[output] = CLOCK_PERIOD - SETUP_TIME;  // Set the required time for outputs

        std::string output_name = asic.net_dict.count(output) ? asic.net_dict.at(output) : "Unknown";
        if (verbose) {
            std::cout << "Output " << output_name << " (ID: " << output << ") → Required time = "
                      << required_time[output] << "\n";
        }
    }

    // Initialize required time for non-output cells (if not already set)
    for (const auto& cell_pair : cell_map) {
        int cell_id = cell_pair.first;

        // Skip if it's an output cell already initialized
        if (required_time.find(cell_id) != required_time.end()) {
            continue;
        }

        // Initialize the required time for non-output cells to a very large number
        required_time[cell_id] = INT64_MAX;  // or use a very large float value

        // Optionally, you can print for debugging
        std::string cell_name = asic.net_dict.count(cell_id) ? asic.net_dict.at(cell_id) : "Unknown";
        if (verbose) {
            std::cout << "Cell " << cell_name << " (ID: " << cell_id << ") → Required time initialized to "
                      << required_time[cell_id] << "\n";
        }
    }
}


void DAG::updateArrivalTime(int current, int neighbor, const std::map<int, Cell>& cell_map) {
    // Find the corresponding delay and slew between current -> neighbor
    double rc_delay = 0;
    double slew = 0;
    for (const auto& [from, to, rc_delay_value] : rc_value) {
        if (from == current && to == neighbor) {
            rc_delay = rc_delay_value;

        }
    }
    for (const auto& [from, to, slew_value_in] : slew_value) {
        if (from == current && to == neighbor) {
            slew = slew_value_in;

        }
    }

    // Fetch the component delays for the current and neighbor cells
    double current_cell_delay = cell_map.at(current).delay;  // Assuming 'delay' is a member of 'Cell'
    double neighbor_cell_delay = cell_map.at(neighbor).delay;  // Assuming 'delay' is a member of 'Cell'

    // Calculate total delay as the sum of RC delay, slew rate, and component delays
    double total_delay = (rc_delay + slew)*10e9 + neighbor_cell_delay;
#pragma omp critical
{
    if (arrival_time.find(neighbor) == arrival_time.end()) {
        arrival_time[neighbor] = 0;
    }

}
    double old_arrival = arrival_time[neighbor];
    double new_arrival = arrival_time[current] + total_delay;



    // Update the neighbor's arrival time with the maximum of the old or new arrival time
#pragma omp critical
{
    arrival_time[neighbor] = std::max(old_arrival, new_arrival);
    if(verbose){
        std::cout << "The delay for rc and slew is " << (rc_delay + slew)*10e9 << std::endl;
        std::cout << "Updating arrival time for cell " << neighbor
        << ": max(" << old_arrival << ", "
        << new_arrival
        << ") = " << arrival_time[neighbor] << std::endl;

    }
}

    return;

    // std::cerr << "Warning: No delay/slew entry found for edge " << current << " -> " << neighbor << std::endl;
}


double DAG::computeSlewRate(const Cell& current_cell, const Cell& neighbor_cell, int current_cell_id, int neighboor_cell_id) {
    // Assuming voltage swing (V) is a constant value, e.g., 1V (you can adjust this value)
    double rc_delay = 0.0;
    double voltage_swing = 1.0; // V
    for (const auto& [from, to, rc_delay_value] : rc_value) {
        if (from == current_cell_id && to == neighboor_cell_id) {
            rc_delay = rc_delay_value;

        }
    }

    // Calculate slew rate based on the RC time constant
    double rc_time_constant = rc_delay;
    double slew_rate = voltage_swing / rc_time_constant;
    double slew_time = voltage_swing / slew_rate; // (V / (V/s)) = seconds
#pragma omp critical
{ 
    slew_value.push_back({current_cell_id, neighboor_cell_id, slew_time});
}
    // Print Slew Rate
    if(verbose)
    {
        std::cout << "Computing Slew Rate: "
        << "Resistance of current cell = " << current_cell.resistance
        << ", Capacitance of neighbor cell = " << neighbor_cell.capacitance
        << " => Slew Rate = " << slew_time << " V/s" << std::endl;
    }
    return slew_time;
}

// Function to compute RC delay between two cells
double DAG:: computeRCDelay(const Cell& current_cell, const Cell& neighbor_cell,int current_id, int neighbor_id) {
    // Calculate RC delay based on the resistance and capacitance of both cells
    double rc_delay = current_cell.resistance * neighbor_cell.capacitance;
#pragma omp critical
{
    rc_value.push_back({current_id, neighbor_id, rc_delay});
}
    // Print RC delay

    if(verbose){
        std::cout << "Computing RC Delay: "
        << "Resistance of current cell = " << current_cell.id
        << ", Capacitance of neighbor cell = " << neighbor_cell.capacitance
        << " => RC Delay = " << rc_delay << std::endl;
    }
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