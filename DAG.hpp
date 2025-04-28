#ifndef DAG_HPP
#define DAG_HPP
#define CLOCK_PERIOD 50
#define SETUP_TIME   8  //time that DFF input needs to be stable before the clock edge
#define HOLD_TIME    4  //time that DFF input needs to be stable after the clock edge
#define CLK2Q_MIN    1 //fastest time between clock edge and DFF output changing
#define CLK2Q_MAX    5 //slowest time between clock edge and DFF output changing
#define CLK_SKEW_MAX 3 //slowest time between clock edge and DFF output changing


#include <vector>
#include <map>
#include <queue>
#include <unordered_set>

#include "ASIC.hpp" 
#include "verbose.h"

class DAG {
private:
    std::map<int, std::vector<int>> adjList; // Adjacency list for the graph
    std::unordered_map<int, std::vector<int>> reverseAdjList; // Reverse adjacency list for the graph
    std::unordered_map<int, float> arrival_time;
    // Function to reverse the adjacency list
    void reverseList();


public:

    struct rcInfo {
        int current_cell_id; // ID of the current cell
        int neighbor_cell_id; // ID of the neighbor cell
        double rc_delay; // RC delay value
    };

    struct slewInfo {
        int current_cell_id; // ID of the current cell
        int neighbor_cell_id; // ID of the neighbor cell
        double slew_rate; // Slew rate value
    };
    // Adds a directed edge from 'from' node to 'to' node
    void addEdge(int from, int to);
    void createTaskGraph();

    // Displays the DAG (adjacency list representation)
    void displayGraph(const ASIC& asic);

    // Builds the DAG based on the provided ASIC object
    void buildFromASIC(const ASIC& asic);
    void removeCycles(); // Performs topological sort on the DAG and returns the sorted order
    std::vector<int> topologicalSort(const ASIC& asic, const std::map<int, Cell>& cell_map);   
    void updateArrivalTime(int current, int neighbor, const std::map<int, Cell>& cell_map);
    double computeRCDelay(const Cell& current_cell, const Cell& neighbor_cell,int current_id, int neighbor_id);
    double computeSlewRate(const Cell& current_cell, const Cell& neighbor_cell, int current_cell_id, int neighboor_cell_id);
    std::unordered_map<int, float> analyzeTiming(const ASIC& asic, const std::map<int, Cell>& cell_map, std::vector<int> &sorted);
    std::unordered_map<int, double> rc_delay_map; // node_id → RC delay
    std::vector<slewInfo> slew_value;
    std::vector<rcInfo> rc_value;
    std::vector<int> topological_TaskGraph(DAG& dag,const std::map<int, Cell>& cell_map);
    std::map<std::string, std::vector<std::string>> taskGraph;
    void processQueue(const std::string& task, DAG& dag,const std::map<int, Cell>& cell_map);
    void printTaskGraph();


};

#endif // DAG_HPP