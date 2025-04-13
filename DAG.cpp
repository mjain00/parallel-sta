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


