#include <vector>
#include <string>
#include "CellType.hpp"

using namespace std;

typedef struct
{
    double delay;
    CellType type;
    vector<string> inputs;
    vector<string> outputs;
} Cell;