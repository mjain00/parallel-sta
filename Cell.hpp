#include <vector>
#include <string>
#include "CellType.hpp"

using namespace std;

typedef struct
{
    int id;
    double delay;
    CellType type;
    vector<int> inputs;
    vector<int> outputs;
} Cell;