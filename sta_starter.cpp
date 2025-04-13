#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <random>
#include <cmath>
#include "ASIC.hpp"

int main()
{
    // get cmd line argument

    // parse json file
    std::cout << "Hello World!" << std::endl;
    ASIC asic = parse_json("simple.json");
    std::cout << "BYE!" << std::endl;
}