#include <vector>
#include <string>
#include "CellType.hpp"

using namespace std;

typedef struct
{
    double delay;
    CellType type;
    vector<int> inputs;
    vector<int> outputs;
} Cell;