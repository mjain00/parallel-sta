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
    // ASIC asic = parse_json("multiout.json");
    ASIC asic = parse_json("simple.json");
    display_asic(asic);
    DAG dag;
    dag.buildFromASIC(asic);
    std::cout << "DAG Representation of the ASIC:" << std::endl;
    dag.displayGraph(asic);
    std::vector<int> sorted = dag.topologicalSort();
    std::cout << "\nTopological Order:\n";
    std::cout << "BYE!" << std::endl;
}