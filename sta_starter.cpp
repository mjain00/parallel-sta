#include <iostream>
#include <fstream>
#include <chrono>
#include "DAG.hpp"
#include "verbose.h"

using namespace std::chrono;

int main(int argc, char **argv)
{
    std::cout << "Static Timing Analysis" << std::endl;

    string filename = "circuits/json/simple.json";

    if (argc > 1 && argc < 4)
    {
        for (int i = 1; i < argc; ++i)
        {
            if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0)
            {
                verbose = true;
            }
            else
            {
                filename = argv[i];
            }
        }
    }
    else if (argc > 3)
    {
        std::cerr << "Usage: " << argv[0] << " [-v|--verbose] <filename>" << std::endl;
        return 1;
    }

    // Timing variables
    std::chrono::high_resolution_clock::time_point start, end;
    long long parse_json_duration, create_cell_map_duration, build_dag_duration;
    long long topological_sort_duration, analyze_timing_duration;

    // Parsing JSON
    start = high_resolution_clock::now();
    ASIC asic = parse_json(filename);
    assign_rc_to_cells(asic);
    end = high_resolution_clock::now();
    parse_json_duration = duration_cast<microseconds>(end - start).count();
    std::cout << "\n[Time] Parsing JSON: " << parse_json_duration << " us" << std::endl;

    // Creating cell map
    start = high_resolution_clock::now();
    std::map<int, Cell> cell_map = create_cell_map(asic.cells);  // Specified map type
    end = high_resolution_clock::now();
    create_cell_map_duration = duration_cast<microseconds>(end - start).count();
    std::cout << "\n[Time] Creating Cell Map: " << create_cell_map_duration << " us" << std::endl;

    // Building DAG
    DAG dag;
    start = high_resolution_clock::now();
    dag.buildFromASIC(asic);
    end = high_resolution_clock::now();
    build_dag_duration = duration_cast<microseconds>(end - start).count();
    std::cout << "\n[Time] Building DAG: " << build_dag_duration << " us" << std::endl;

    std::cout << "\nDAG Representation of the ASIC:" << std::endl;
    dag.displayGraph(asic);
    dag.removeCycles();
    // dag.createTaskGraph();
    // dag.printTaskGraph();

    // Topological Sort (Forward Pass)
    start = high_resolution_clock::now();
    std::vector<int> sorted = dag.topologicalSort(asic, cell_map);  // Specified vector type
    end = high_resolution_clock::now();
    topological_sort_duration = duration_cast<microseconds>(end - start).count();
    std::cout << "\n[Time] Topological Sort (Forward Pass): " << topological_sort_duration << " us" << std::endl;

    // Timing analysis (Backward Pass)
    start = high_resolution_clock::now();
    std::unordered_map<int, float> slack = dag.analyzeTiming(asic, cell_map, sorted);  // Specified unordered_map type
    end = high_resolution_clock::now();
    analyze_timing_duration = duration_cast<microseconds>(end - start).count();
    std::cout << "\n[Time] Analyze Timing (Backward Pass): " << analyze_timing_duration << " us" << std::endl;

    // Results
    std::cout << "\nRESULTS:" << std::endl;

    for (const std::pair<int, float>& entry : slack)  // Explicitly specify pair type
    {
        const int& net = entry.first;
        const float& s = entry.second;

        std::string name = asic.net_dict.count(net) ? asic.net_dict.at(net) : "Unknown";
        std::cout << "Node " << name << " (ID: " << net << ") | Slack: " << s;

        if (s < 0)
        {
            std::cout << " | Timing Violation!";
        }
        else
        {
            std::cout << " | Timing OK!";
        }

        std::cout << std::endl;
    }

    // Displaying the times at the end
    std::cout << "\n[Time Summary]" << std::endl;
    std::cout << "Parsing JSON: " << parse_json_duration << " us" << std::endl;
    std::cout << "Creating Cell Map: " << create_cell_map_duration << " us" << std::endl;
    std::cout << "Building DAG: " << build_dag_duration << " us" << std::endl;
    std::cout << "Topological Sort (Forward Pass): " << topological_sort_duration << " us" << std::endl;
    std::cout << "Analyze Timing (Backward Pass): " << analyze_timing_duration << " us" << std::endl;

    std::cout << "BYE!" << std::endl;

    return 0;
}
