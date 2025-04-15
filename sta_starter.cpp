#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <random>
#include <cmath>
// #include "ASIC.hpp"
#include "DAG.hpp"

// #include "DAG.hpp"


int main()
{
    // get cmd line argument

    // parse json file
    std::cout << "Hello World!" << std::endl;
    ASIC asic = parse_json("simple.json");
    // ASIC asic = parse_json("simple.json");
    display_asic(asic);
    map<int, Cell> cell_map = create_cell_map(asic.cells);
    DAG dag;
    dag.buildFromASIC(asic);
    std::cout << "DAG Representation of the ASIC:" << std::endl;
    dag.displayGraph(asic);
    std::cout << "\nTopological Order:\n";
    std::vector<int> sorted = dag.topologicalSort(asic, cell_map);

    dag.analyzeTiming(asic, cell_map, sorted);


    
    std::cout << "BYE!" << std::endl;
}