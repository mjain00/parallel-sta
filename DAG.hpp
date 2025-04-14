#ifndef DAG_HPP
#define DAG_HPP

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
    std::vector<int> topologicalSort(const ASIC& asic);

};

#endif // DAG_HPP
