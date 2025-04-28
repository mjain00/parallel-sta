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

std::vector<std::vector<int>> DAG::createLevelList(const ASIC &asic, const std::map<int, Cell> &cell_map)
{
    std::unordered_map<int, int> inDegree;
    std::vector<std::vector<int>> levelList;
    std::unordered_map<int, int> level;
    std::queue<int> q;
    // Calculate in-degrees
    for (const auto &node : adjList)
    {
        inDegree[node.first]; // Defaults to 0 if not present
        for (int neighbor : node.second)
        {
            inDegree[neighbor]++;
        }
    }

    // Enqueue nodes with 0 in-degree
    for (const auto &[node, degree] : inDegree)
    {
        if (degree == 0)
        {
            q.push(node);
            level[node] = 0;
        }
    }

    int max_level = 0;
    while (!q.empty())
    {
        int current = q.front();
        q.pop();
        int current_level = level[current];
        max_level = std::max(max_level, current_level);

        // For each outgoing connection from 'current', calculate RC delay
        for (int neighbor : adjList[current])
        {
            inDegree[neighbor]--;
            if (inDegree[neighbor] == 0)
            {
                q.push(neighbor);
                level[neighbor] = current_level + 1;
            }
        }
    }

    levelList.resize(max_level + 1);
    for (const auto &[node, l] : level)
    {
        levelList[l].push_back(node);
    }

    return levelList;
}

void DAG::propagateRC(int node, const std::map<int, Cell> &cell_map)
{
    // std::cout << "TID: " << omp_get_thread_num() << " - in propRC for node: " << node << std::endl;
    if (cell_map.find(node) == cell_map.end())
        return;

    for (int neighbor : adjList[node])
    {
        if (cell_map.find(neighbor) == cell_map.end())
            continue;

        double rc_delay = computeRCDelay(cell_map.at(node), cell_map.at(neighbor));

#pragma omp critical
        {
            edge_timings[node][neighbor].rc_delay = rc_delay;
            // std::cout << "TID: " << omp_get_thread_num() << " - updating rc delay map for node: " << node << " to " << rc_delay << std::endl;
        }
    }
}

void DAG::propagateSlew(int node, const std::map<int, Cell> &cell_map)
{
    // std::cout << "TID: " << omp_get_thread_num() << " - in propSlew for node: " << node << std::endl;
    if (cell_map.find(node) == cell_map.end())
        return;

    for (int neighbor : adjList[node])
    {
        if (cell_map.find(neighbor) == cell_map.end())
            continue;

        double rc_delay = 0.0;
#pragma omp critical
        {
            rc_delay = edge_timings[node][neighbor].rc_delay;
        }

        // std::cout << "TID: " << omp_get_thread_num() << " - in propSlew RC delay for node: " << node << " is: " << rc_delay << std::endl;

        double slew_rate = computeSlewRate(cell_map.at(node), cell_map.at(neighbor), rc_delay);

#pragma omp critical
        {
            edge_timings[node][neighbor].slew_rate = slew_rate;
            // std::cout << "TID: " << omp_get_thread_num() << " - Propagating Slew for node: " << node << std::endl;
        }
    }
}

void DAG::propagateArrivalTime(int node, const std::map<int, Cell> &cell_map)
{
    // std::cout << "TID: " << omp_get_thread_num() << " - in propArrivalTime for node: " << node << std::endl;
    if (cell_map.find(node) == cell_map.end())
        return;

    for (int neighbor : adjList[node])
    {
        if (cell_map.find(neighbor) == cell_map.end())
            continue;
        EdgeTiming et;

#pragma omp critical
        {
            et = edge_timings[node][neighbor];
        }
        double cell_delay = cell_map.at(neighbor).delay;

        double total_delay = (et.rc_delay + et.slew_rate) * 10e9 + cell_delay;

#pragma omp critical
        {
            double old_arrival = arrival_time[neighbor];
            double new_arrival = arrival_time[node] + total_delay;

            // Update the neighbor's arrival time with the maximum of the old or new arrival time
            arrival_time[neighbor] = std::max(old_arrival, new_arrival);
            if (verbose)
                std::cout << "Updating arrival time for cell " << neighbor
                          << ": max(" << old_arrival << ", "
                          << arrival_time[node] << " + " << total_delay
                          << ") = " << arrival_time[neighbor] << std::endl;
        }

        // updateArrivalTime(node, neighbor, cell_map);
        // std::cout << "TID: " << omp_get_thread_num() << " - Propagating Arrival Time for node: " << node << std::endl;
    }
}

void DAG::forwardPropagation(const ASIC &asic, const std::map<int, Cell> &cell_map, std::vector<std::vector<int>> level_list)
{
    int l_min = 0;
    int l_max = level_list.size() - 1;
    while (l_max >= 0 && level_list[l_max].empty())
    {
        --l_max;
    }

    // std::cout << "lmin: " << l_min << " lmax: " << l_max << std::endl;
    std::vector<int> rc_dep(l_max + 1, 0);
    std::vector<int> slew_dep(l_max + 1, 0);

#pragma omp parallel num_threads(16)
    {
#pragma omp single
        { // master thread creates task s
            // std::cout << "TID: " << omp_get_thread_num() << " creating tasks " << std::endl;
            for (int i = l_min; i < l_max + 2; ++i)
            {
                if (i <= l_max)
                {
                    for (int u : level_list[i])
                    {
                        // std::cout << "task - Propagating RC for node: " << u << std::endl;
#pragma omp task firstprivate(u, i) // depend(out : rc_dep.data()[i])
                        {
                            propagateRC(u, cell_map);
                        }
                    }
                }

                if (i - 1 >= l_min)
                {
                    for (int u : level_list[i - 1])
                    {
                        // std::cout << "Task - Propagating Slew for node: " << u << " in level " << (i - 1) << std::endl;
#pragma omp task firstprivate(u, i) // depend(in: rc_dep.data()[i]) depend(out: slew_dep.data()[i]) //depend(inout : i) // depend(in : level_list[i]) depend(out : level_list[i - 1])
                        {
                            propagateSlew(u, cell_map);
                        }
                    }
                }

                if (i - 2 >= l_min)
                {
                    for (int u : level_list[i - 2])
                    {
                        // std::cout << "Task - Propagating Arrival Time for node: " << u << " in level " << (i - 1) << std::endl;
#pragma omp task firstprivate(u, i) // depend(in: slew_dep.data()[i - 2]) //depend(inout : i) // depend(in : level_list[i]) depend(out : level_list[i - 2])
                        {
                            propagateArrivalTime(u, cell_map);
                        }
                    }
                }
#pragma omp taskwait
            }
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
                double slew_rate = computeSlewRate(cell_map.at(current), cell_map.at(neighbor),rc_delay);
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
            double current_cell_delay = cell_map.at(current).delay;  // Assuming 'delay' is a member of 'Cell'
            double neighbor_cell_delay = cell_map.at(neighbor).delay;  // Assuming 'delay' is a member of 'Cell'

            // Calculate total delay as the sum of RC delay, slew rate, and component delays
            double total_delay = (rc_delay + slew)*10e9 + neighbor_cell_delay;

            double old_arrival = arrival_time[neighbor];
            double new_arrival = arrival_time[current] + total_delay;

            // Update the neighbor's arrival time with the maximum of the old or new arrival time
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


double DAG::computeSlewRate(const Cell& current_cell, const Cell& neighbor_cell,double rc_delay) {
    // Assuming voltage swing (V) is a constant value, e.g., 1V (you can adjust this value)
    double voltage_swing = 1.0; // V

    // Calculate slew rate based on the RC time constant
    double rc_time_constant = rc_delay;
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

void DAG::propagateFanin(int node, const std::map<int, Cell> &cell_map, const ASIC &asic)
{
    if (reverseAdjList.count(node) == 0)
        return;

    if (required_time.find(node) == required_time.end())
    {
        return;
        // required_time[node] = INT32_MAX;
        // std::cout << "TID: " << omp_get_thread_num() << " set req time for node " << node << " to max val\n";
    }

    float cell_delay = 0.0f;

    if (cell_map.count(node))
    {
        cell_delay = cell_map.at(node).delay;
    }
    for (int fanin : reverseAdjList[node])
    {
        int candidate_time = required_time[node] - cell_delay;
        // std::cout << "TID: " << omp_get_thread_num() << " candidate time for node " << node << " is " << candidate_time << endl;

#pragma omp critical
        {
            if (required_time.find(fanin) == required_time.end())
            {
                required_time[fanin] = candidate_time;
                // std::cout << "TID: " << omp_get_thread_num() << " set req time for node " << node << " to candidate value\n";
            }
            else
            {
                required_time[fanin] = std::min(required_time[fanin], candidate_time);
                // std::cout << "TID: " << omp_get_thread_num() << " set req time for node " << node << " to min val\n";
            }
        }

        if (verbose)
        {
            std::string fanin_name = asic.net_dict.count(fanin) ? asic.net_dict.at(fanin) : "Unknown";

            std::cout << "TID: " << omp_get_thread_num() << "  Fanin " << fanin_name << " (ID: " << fanin
                      << ") → Required time updated to " << required_time[fanin]
                      << " (via " << cell_delay << " delay)\n";
        }
    }
}

void DAG::propagateRequiredArrivalTime(int node, const std::map<int, Cell> &cell_map, const ASIC &asic)
{
    float at = arrival_time.count(node) ? arrival_time.at(node) : 0.0f;
    float rt = required_time.count(node) ? required_time.at(node) : CLOCK_PERIOD;

    float s = rt - at;

#pragma omp critical
    {
        slack[node] = s;
    }

    if (verbose)
    {
        std::string net_name = asic.net_dict.count(node) ? asic.net_dict.at(node) : "Unknown";
        std::cout << "Net " << net_name << " (ID: " << node << ")"
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

void DAG::backwardPropagation(const ASIC &asic, const std::map<int, Cell> &cell_map, std::vector<std::vector<int>> &level_list)
{
    int l_min = 0;
    int l_max = level_list.size() - 1;
    while (l_max >= 0 && level_list[l_max].empty())
    {
        --l_max;
    }

    reverseList();

    for (int output : asic.outputs)
    {
        required_time[output] = CLOCK_PERIOD - SETUP_TIME;

        if (verbose)
        {
            std::string name = asic.net_dict.count(output) ? asic.net_dict.at(output) : "Unknown";

            std::cout << "Output net " << name << " (ID: " << output << ") → Required time = "
                      << required_time[output] << "\n";
        }
    }

#pragma omp parallel num_threads(16)
    {
#pragma omp single
        {
            for (int i = l_max; i >= l_min; i--)
            {
// propagate Fanin
#pragma omp task firstprivate(i)
                {
                    for (int u : level_list[i])
                    {
                        propagateFanin(u, cell_map, asic);
                    }
                }
                // propagate required arrival time
                // #pragma omp taskwait

#pragma omp task firstprivate(i)
                {
                    for (int u : level_list[i])
                    {
                        propagateRequiredArrivalTime(u, cell_map, asic);
                    }
                }
#pragma omp taskwait
            }
        }
    }
}

std::unordered_map<int, float> DAG::calculateSlack(const ASIC &asic, const std::map<int, Cell> &cell_map, std::vector<std::vector<int>> &level_list)
{
    // std::unordered_map<int, int> required_time;
    std::unordered_map<int, float> slack;

    reverseList();

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

    for (int i = level_list.size() - 1; i >= 0; i--)
    {
        for (auto it = level_list[i].rbegin(); it != level_list[i].rend(); ++it)
        {
            int current = *it;
            // std::cout << "Current node: " << current << std::endl;

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
    }

    for (int i = 0; i < level_list.size(); i++)
    {
        for (int net : level_list[i])
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
    }
    return slack;
}

std::unordered_map<int, float> DAG::getSlack()
{
<<<<<<< Updated upstream
=======
    return slack;
}





std::unordered_map<int, float> DAG::analyzeTiming(const ASIC& asic, const std::map<int, Cell>& cell_map, std::vector<int> &sorted) {
>>>>>>> Stashed changes
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

<<<<<<< Updated upstream
        if (required_time.find(current) == required_time.end())
        {
            required_time[current] = INT32_MAX;
=======
        if (required_time.find(current) == required_time.end()) {
            required_time[current] = INT64_MAX;
>>>>>>> Stashed changes
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
<<<<<<< Updated upstream
}

std::unordered_map<int, float> DAG::getSlack()
{
    return slack;
=======
>>>>>>> Stashed changes
}