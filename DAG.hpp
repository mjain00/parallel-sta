#ifndef DAG_HPP
#define DAG_HPP
#define CLOCK_PERIOD 50
#define SETUP_TIME 8   // time that DFF input needs to be stable before the clock edge
#define HOLD_TIME 4    // time that DFF input needs to be stable after the clock edge
#define CLK2Q_MIN 1    // fastest time between clock edge and DFF output changing
#define CLK2Q_MAX 5    // slowest time between clock edge and DFF output changing
#define CLK_SKEW_MAX 3 // slowest time between clock edge and DFF output changing

#include <vector>
#include <map>
#include <queue>
#include <unordered_set>
#include <omp.h>

#include "ASIC.hpp"
#include "verbose.h"

class DAG
{
private:
    std::map<int, std::vector<int>> adjList;                  // Adjacency list for the graph
    std::unordered_map<int, std::vector<int>> reverseAdjList; // Reverse adjacency list for the graph
    std::unordered_map<int, float> arrival_time;
    std::unordered_map<int, int> required_time;
    std::unordered_map<int, float> slack;
    // Function to reverse the adjacency list
    void reverseList();
    void propagateRC(int node, const std::map<int, Cell> &cell_map);
    void propagateSlew(int node, const std::map<int, Cell> &cell_map);
    void propagateArrivalTime(int node, const std::map<int, Cell> &cell_map);
    void propagateDelay(int node, const std::map<int, Cell> &cell_map);
    void propagateFanin(int node, const std::map<int, Cell> &cell_map, const ASIC &asic);
    void propagateRequiredArrivalTime(int node, const std::map<int, Cell> &cell_map, const ASIC &asic);

public:
    struct DelaySlewInfo
    {
        int current_cell_id;  // ID of the current cell
        int neighbor_cell_id; // ID of the neighbor cell
        double rc_delay;      // RC delay value
        double slew_rate;     // Slew rate value
    };

    struct EdgeTiming
    {
        double rc_delay;
        double slew_rate;
        double total_delay;
    };
    // Adds a directed edge from 'from' node to 'to' node
    void addEdge(int from, int to);
    void createTaskGraph();

    // Displays the DAG (adjacency list representation)
    void displayGraph(const ASIC &asic);

    // Builds the DAG based on the provided ASIC object
    void buildFromASIC(const ASIC &asic);
    void removeCycles(); // Performs topological sort on the DAG and returns the sorted order
    std::vector<int> topologicalSort(const ASIC &asic, const std::map<int, Cell> &cell_map);
    std::vector<std::vector<int>> createLevelList(const ASIC &asic, const std::map<int, Cell> &cell_map);
    void forwardPropagation(const ASIC &asic, const std::map<int, Cell> &cell_map, std::vector<std::vector<int>> level_list);
    void updateArrivalTime(int current, int neighbor, const std::map<int, Cell> &cell_map);
    double computeRCDelay(const Cell &current_cell, const Cell &neighbor_cell);
    double computeSlewRate(const Cell &current_cell, const Cell &neighbor_cell, double rc_delay);
    std::unordered_map<int, float> analyzeTiming(const ASIC &asic, const std::map<int, Cell> &cell_map, std::vector<int> &sorted);
    std::unordered_map<int, float> calculateSlack(const ASIC &asic, const std::map<int, Cell> &cell_map, std::vector<std::vector<int>> &level_list);
    void backwardPropagation(const ASIC &asic, const std::map<int, Cell> &cell_map, std::vector<std::vector<int>> &level_list);
    std::unordered_map<int, std::unordered_map<int, EdgeTiming>> edge_timings; // node_id → RC delay
    std::vector<DelaySlewInfo> delays_and_slews;
    std::map<std::string, std::vector<std::string>> taskGraph;
    void printTaskGraph();
    std::unordered_map<int, float> getSlack();
};

#endif // DAG_HPP
