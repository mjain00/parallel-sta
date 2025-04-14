#ifndef DAG_HPP
#define DAG_HPP
#define CLOCK_PERIOD = 50
#define SETUP_TIME   = 8  //time that DFF input needs to be stable before the clock edge
#define HOLD_TIME    = 4  //time that DFF input needs to be stable after the clock edge
#define CLK2Q_MIN    = 1 //fastest time between clock edge and DFF output changing
#define CLK2Q_MAX    = 5 //slowest time between clock edge and DFF output changing
#define CLK_SKEW_MAX = 3 //slowest time between clock edge and DFF output changing


#include <vector>
#include <map>
#include <queue>

#include "ASIC.hpp"  // Ensure Cell.hpp is defined correctly.

class DAG {
private:
    std::map<int, std::vector<int>> adjList; // Adjacency list for the graph
    std::map<int, int> netToCell; // Mapping from net ID to cell ID

public:
    // Adds a directed edge from 'from' node to 'to' node
    void addEdge(int from, int to);

    // Displays the DAG (adjacency list representation)
    void displayGraph(const ASIC& asic);

    // Builds the DAG based on the provided ASIC object
    void buildFromASIC(const ASIC& asic);
    std::vector<int> topologicalSort(const ASIC& asic, const std::map<int, Cell>& cell_map);
};

#endif // DAG_HPP
