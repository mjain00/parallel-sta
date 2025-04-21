#include <map>
#include <string>
#include <unordered_map>

using namespace std;

enum class CellType {
    UNKNOWN,
    NOT,
    AND,
    OR,
    XOR,
    NAND,
    NOR,
    XNOR,
    DFF_P,
    DFF_PN0,
    MUX,
    AND2_X2, AND2_X4, AND3_X2, AND3_X4, AND4_X2, AND4_X4,
    AOI211_X2, AOI21_X2, AOI21_X4, AOI221_X2,
    AOI222_X1, AOI222_X2, AOI22_X2,
    CLKBUF_X1, CLKBUF_X2,
    DFFR_X1, DFFR_X2, DFFS_X2, DFF_X1, DFF_X2,
    HA_X1,
    INV_X1, INV_X16, INV_X2, INV_X32, INV_X4, INV_X8,
    MUX2_X1,
    NAND2_X1, NAND2_X2, NAND2_X4, NAND3_X2, NAND3_X4, NAND4_X2,
    NOR2_X2, NOR2_X4, NOR3_X2, NOR3_X4, NOR4_X2,
    OAI211_X2, OAI21_X2, OAI221_X2, OAI222_X2, OAI22_X1, OAI22_X2,
    OR2_X2, OR2_X4,
    SDFFR_X2, SDFF_X2,
    XNOR2_X2, XOR2_X2
};