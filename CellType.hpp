#include <map>
#include <string>
#include <unordered_map>

using namespace std;

enum class CellType
{
    NOT,
    NAND,
    NOR,
    AND,
    OR,
    XOR,
    XNOR,
    DFF_P,
    UNKNOWN
};
