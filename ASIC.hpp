#include <iostream>
#include <vector>
#include <string>
#include <map>
#include "external/json/json.hpp"
#include "Cell.hpp"
using namespace std;

typedef struct
{
    vector<int> inputs;
    vector<int> outputs;
    vector<Cell> netlist;
    map<int, string> net_dict;
    // vector<string> paths; //come back to this 
} ASIC;

void display_asic(const ASIC &asic);
ASIC parse_json(const string &filename);
void print_cells(const ASIC &asic);
