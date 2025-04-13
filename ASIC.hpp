#include <iostream>
#include <vector>
#include <string>
#include <map>
#include "external/json/json.hpp"
#include "Cell.hpp"
using namespace std;

typedef struct
{
    vector<string> inputs;
    vector<string> outputs;
    vector<Cell> cells;
    map<string, Cell> net_dict;
    vector<string> paths;
} ASIC;

void display_asic(const ASIC &asic);
ASIC parse_json(const string &filename);