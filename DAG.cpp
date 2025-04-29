#include "DAG.hpp"
#include <iostream>
#include <chrono>

// Adds a directed edge from 'from' to 'to'
void DAG::addEdge(int from, int to)
{
    adjList[from].push_back(to);
}

void DAG::createTaskGraph()
{
    for (const auto &[node, neighbors] : adjList)
    {
        std::string rc = std::to_string(node) + "_rc";
        std::string slew = std::to_string(node) + "_slew";
        std::string arr = std::to_string(node) + "_arrival";

        taskGraph[rc].push_back(slew);
        taskGraph[slew].push_back(arr);

        for (int neighbor : neighbors)
        {
            std::string neighbor_rc = std::to_string(neighbor) + "_rc";
            taskGraph[arr].push_back(neighbor_rc);
        }
    }
}

void DAG::printTaskGraph()
{
    std::cout << "Task Graph Dependencies (DAG):\n";
    for (const auto &[task, dependents] : taskGraph)
    {
        std::cout << task << " -> ";
        if (dependents.empty())
        {
            std::cout << "{}";
        }
        else
        {
            for (const auto &dep : dependents)
            {
                std::cout << dep << " ";
            }
        }
        std::cout << "\n";
    }
}

void DAG::displayGraph(const ASIC &asic)
{
    for (const auto &node : adjList)
    {
        // Get the net name from net_dict using the node's ID
        std::string node_name = (asic.net_dict.find(node.first) != asic.net_dict.end())
                                    ? asic.net_dict.at(node.first)
                                    : "Unknown";

        std::cout << "Node " << node_name << " (ID: " << node.first << ") has edges to: ";

        for (const auto &neighbor : node.second)
        {
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
void DAG::buildFromASIC(const ASIC &asic)
{
    for (const auto &cell : asic.cells)
    {
        for (const auto &input : cell.inputs)
        {
            for (const auto &output : cell.outputs)
            {
                addEdge(input, output); // Create edges based on the cell's inputs and outputs
            }
        }
    }
}

void DAG::removeCycles()
{
    std::unordered_set<int> visited;
    std::unordered_set<int> recStack;

    std::function<void(int)> dfs = [&](int node)
    {
        visited.insert(node);
        recStack.insert(node);

        auto &neighbors = adjList[node]; // direct reference to the neighbor set
        for (auto it = neighbors.begin(); it != neighbors.end();)
        {
            int neighbor = *it;

            if (recStack.count(neighbor))
            {
                // Found a back edge → remove it
                std::cout << "Removing back edge: " << node << " -> " << neighbor << "\n";
                it = neighbors.erase(it); // erase and continue
            }
            else if (!visited.count(neighbor))
            {
                dfs(neighbor);
                ++it;
            }
            else
            {
                ++it;
            }
        }

        recStack.erase(node);
    };

    for (const auto &[node, _] : adjList)
    {
        if (!visited.count(node))
        {
            dfs(node);
        }
    }
}

std::vector<int> DAG::topologicalSort(const ASIC &asic, const std::map<int, Cell> &cell_map)
{
    std::unordered_map<int, int> inDegree;
    std::unordered_map<int, double> local_arrival_time;
    std::vector<int> result;
    std::vector<int> q;

    // Step 1: Calculate in-degrees
    for (const auto &node : adjList)
    {
        if (!inDegree.count(node.first))
            inDegree[node.first] = 0;
        for (int neighbor : node.second)
        {
            inDegree[neighbor]++;
        }
    }

    // Step 2: Enqueue all nodes with in-degree 0
    for (const auto &[node, deg] : inDegree)
    {
        if (deg == 0)
        {
            q.push_back(node);
            local_arrival_time[node] = 0.0;
        }
    }

    while (!q.empty())
    {
        std::vector<int> next_q;
        std::vector<int> q_copy = q;

#pragma omp parallel num_threads(8)
        {
            std::vector<int> local_next;
            std::vector<std::tuple<int, int, double, double>> local_delays;

#pragma omp for
            for (int i = 0; i < q_copy.size(); ++i)
            {
                int current = q_copy[i];

#pragma omp critical
                {
                    result.push_back(current);
                }

                for (int neighbor : adjList.at(current))
                {
                    if (cell_map.find(current) != cell_map.end() && cell_map.find(neighbor) != cell_map.end())
                    {
                        double rc_delay = computeRCDelay(cell_map.at(current), cell_map.at(neighbor));
                        double slew = computeSlewRate(cell_map.at(current), cell_map.at(neighbor), rc_delay);
#pragma omp critical
                        {
                            delays_and_slews.push_back({current, neighbor, rc_delay, slew});
                        }
                        updateArrivalTime(current, neighbor, cell_map); // Make this thread-safe or reduce inside parallel block
                    }

#pragma omp atomic
                    --inDegree[neighbor];

                    int updated_deg;
#pragma omp atomic read
                    updated_deg = inDegree[neighbor];

                    if (updated_deg == 0)
                    {
                        local_next.push_back(neighbor);
                    }
                }
            }
#pragma omp critical
            {
                next_q.insert(next_q.end(), local_next.begin(), local_next.end());
            }
        }

        q = next_q;
    }

    if (result.empty())
    {
        std::cerr << "\nError: No nodes processed. Possible cycle in graph.\n";
    }

    return result;
}

void DAG::updateArrivalTime(int current, int neighbor, const std::map<int, Cell> &cell_map)
{
    for (const auto &[from, to, rc_delay, slew] : delays_and_slews)
    {
        if (from == current && to == neighbor)
        {
            double current_cell_delay = cell_map.at(current).delay;
            double neighbor_cell_delay = cell_map.at(neighbor).delay;

            double total_delay = (rc_delay + slew) * 10e9 + neighbor_cell_delay;

// Entire block critical to prevent race conditions
#pragma omp critical
            {
                if (arrival_time.find(current) == arrival_time.end())
                {
                    arrival_time[current] = 0.0;
                }
                if (arrival_time.find(neighbor) == arrival_time.end())
                {
                    arrival_time[neighbor] = 0.0;
                }

                double old_arrival = arrival_time[neighbor];
                double new_arrival = arrival_time[current] + total_delay;
                arrival_time[neighbor] = std::max(old_arrival, new_arrival);
            }
        }
    }
}

double DAG::computeSlewRate(const Cell &current_cell, const Cell &neighbor_cell, double rc_delay)
{
    double voltage_swing = 1.0;

    double rc_time_constant = rc_delay;
    double slew_rate = voltage_swing / rc_time_constant;
    double slew_time = voltage_swing / slew_rate; // (V / (V/s)) = seconds

    if (verbose)
        std::cout << "Computing Slew Rate: "
                  << "Resistance of current cell = " << current_cell.resistance
                  << ", Capacitance of neighbor cell = " << neighbor_cell.capacitance
                  << " => Slew Rate = " << slew_rate << " V/s" << std::endl;

    return slew_time;
}

double DAG::computeRCDelay(const Cell &current_cell, const Cell &neighbor_cell)
{
    double rc_delay = current_cell.resistance * neighbor_cell.capacitance;

    if (verbose)
        std::cout << "Computing RC Delay: "
                  << "Resistance of current cell = " << current_cell.id
                  << ", Capacitance of neighbor cell = " << neighbor_cell.capacitance
                  << " => RC Delay = " << rc_delay << std::endl;

    return rc_delay;
}

void DAG::reverseList()
{
    for (const auto &[u, neighbors] : adjList)
    {
        for (int v : neighbors)
        {
            reverseAdjList[v].push_back(u);
        }
    }
}

std::unordered_map<int, float> DAG::analyzeTiming(const ASIC &asic, const std::map<int, Cell> &cell_map, std::vector<int> &sorted)
{
    std::unordered_map<int, int> required_time;
    std::unordered_map<int, float> slack;

    reverseList();

    if (verbose)
    {
        std::cout << "\n=== Step 1: Initializing required times at outputs ===\n";
    }

    for (int output : asic.outputs)
    {
        required_time[output] = CLOCK_PERIOD - SETUP_TIME;
        std::string name = asic.net_dict.count(output) ? asic.net_dict.at(output) : "Unknown";

        if (verbose)
        {
            std::cout << "Output net " << name << " (ID: " << output << ") → Required time = "
                      << required_time[output] << "\n";
        }
    }

    if (verbose)
    {
        std::cout << "\n=== Step 2: Propagating required times (backward) ===\n";
    }
    for (auto it = sorted.rbegin(); it != sorted.rend(); ++it)
    {
        int current = *it;

        if (required_time.find(current) == required_time.end())
        {
            required_time[current] = INT32_MAX;
        }

        if (reverseAdjList.count(current))
        {
            for (int fanin : reverseAdjList[current])
            {
                float cell_delay = 0.0f;

                // delay from cell driving `current`
                if (cell_map.count(current))
                {
                    cell_delay = cell_map.at(current).delay;
                }

                int candidate_time = required_time[current] - cell_delay;

                if (required_time.find(fanin) == required_time.end())
                {
                    required_time[fanin] = candidate_time;
                }
                else
                {
                    required_time[fanin] = std::min(required_time[fanin], candidate_time);
                }

                std::string fanin_name = asic.net_dict.count(fanin) ? asic.net_dict.at(fanin) : "Unknown";

                if (verbose)
                {
                    std::cout << "  Fanin " << fanin_name << " (ID: " << fanin
                              << ") → Required time updated to " << required_time[fanin]
                              << " (via " << cell_delay << " delay)\n";
                }
            }
        }
    }

    if (verbose)
    {
        std::cout << "\n=== Step 3: Slack Computation ===\n";
    }

    for (int net : sorted)
    {
        float at = arrival_time.count(net) ? arrival_time.at(net) : 0.0f;
        float rt = required_time.count(net) ? required_time[net] : CLOCK_PERIOD;

        float s = rt - at;
        slack[net] = s;

        std::string net_name = asic.net_dict.count(net) ? asic.net_dict.at(net) : "Unknown";

        if (verbose)
        {
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