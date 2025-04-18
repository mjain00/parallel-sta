#include <iostream>
#include <fstream>
#include <chrono>
#include "DAG.hpp"
#include "verbose.h"

using namespace std::chrono;


int main(int argc, char** argv)
{
    std::cout << "Static Timing Analysis" << std::endl;

    string filename = "circuits/json/simple.json";

    if (argc > 1 && argc < 4) {
        for (int i = 1; i < argc; ++i) {
            if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
                verbose = true;
            } else {
                filename = argv[i];
            }
        }
    } else if (argc > 3) {
        std::cerr << "Usage: " << argv[0] << " [-v|--verbose] <filename>" << std::endl;
        return 1;
    }    
    
    auto start = high_resolution_clock::now();

    ASIC asic = parse_json(filename);
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end - start).count();
    cout << "\n[Time] Parsing JSON: " << duration << " us" << endl;

    display_asic(asic);

    start = high_resolution_clock::now();
    map<int, Cell> cell_map = create_cell_map(asic.cells);
    end = high_resolution_clock::now();
    duration = duration_cast<microseconds>(end - start).count();
    cout << "\n[Time] Creating Cell Map: " << duration << " us" << endl;

    DAG dag;

    start = high_resolution_clock::now();
    dag.buildFromASIC(asic);
    end = high_resolution_clock::now();
    duration = duration_cast<microseconds>(end - start).count();
    cout << "\n[Time] Building DAG: " << duration << " us" << endl;

    std::cout << "\nDAG Representation of the ASIC:" << std::endl;
    dag.displayGraph(asic);

    start = high_resolution_clock::now();
    
    std::vector<int> sorted = dag.topologicalSort(asic, cell_map);

    end = high_resolution_clock::now();
    duration = duration_cast<microseconds>(end - start).count();
    cout << "\n[Time] Topological Sort (Forward Pass): " << duration << " us" << endl;

    start = high_resolution_clock::now();

    std::unordered_map<int, float> slack = dag.analyzeTiming(asic, cell_map, sorted);

    end = high_resolution_clock::now();
    duration = duration_cast<microseconds>(end - start).count();
    cout << "\n[Time] Analyze Timing (Backward Pass): " << duration << " us" << endl;

    std::cout << "\nRESULTS:" << std::endl;

    for (const auto& [net, s] : slack) {
        std::string name = asic.net_dict.count(net) ? asic.net_dict.at(net) : "Unknown";
        std::cout << "Node " << name << " (ID: " << net << ") | Slack: " << s;

        if (s < 0) {
            std::cout << " | Timing Violation!";
        } else {
            std::cout << " | Timing OK!";
        }

        std::cout << std::endl;
    }
    std::cout << "BYE!" << std::endl;
}