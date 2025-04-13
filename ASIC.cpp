#include <fstream>
#include <vector>
#include <string>
#include <map>
#include "ASIC.hpp"
using namespace std;
using json = nlohmann::json;

void display_asic(const ASIC &asic)
{
    cout << "ASIC Details:" << endl;

    cout << "Cell List:" << endl;

    for (const auto &cell : asic.cells)
    {
        cout << "Cell Type: " << static_cast<int>(cell.type) << endl;
        cout << "Delay: " << cell.delay << endl;
        cout << "Inputs: ";
        for (const auto &input : cell.inputs)
        {
            cout << input << " ";
        }
        cout << endl;
        cout << "Outputs: ";
        for (const auto &output : cell.outputs)
        {
            cout << output << " ";
        }
        cout << endl;
    }

    cout << "IO list:" << endl;
    cout << "Inputs: ";
    for (const auto &input : asic.inputs)
    {
        cout << input << " ";
    }
    cout << endl;
    cout << "Outputs: ";
    for (const auto &output : asic.outputs)
    {
        cout << output << " ";
    }
    cout << endl;

    cout << "Paths: ";
    for (const auto &path : asic.paths)
    {
        cout << path << " ";
    }
    cout << endl;

    cout << "Net Mappings:" << endl;
    // NEED TO CHECK
    for (const auto &pair : asic.net_dict)
    {
        cout << "[" << pair.first << "] -> ";
        const Cell &cell = pair.second;
        cout << "Type: " << static_cast<int>(cell.type) << " Inputs: ";
        for (const auto &input : cell.inputs)
            cout << input << " ";
        cout << "Outputs: ";
        for (const auto &output : cell.outputs)
            cout << output << " ";
        cout << endl;
    }
}

CellType parse_cell_type(const std::string &type_str)
{
    if (type_str == "$_NOT_")
        return CellType::NOT;
    if (type_str == "$_AND_")
        return CellType::AND;
    if (type_str == "$_OR_")
        return CellType::OR;
    if (type_str == "$_XOR_")
        return CellType::XOR;
    if (type_str == "$_NAND_")
        return CellType::NAND;
    if (type_str == "$_NOR_")
        return CellType::NOR;
    if (type_str == "$_XNOR_")
        return CellType::XNOR;
    if (type_str == "$_DFF_P_")
        return CellType::DFF_P;
    return CellType::UNKNOWN;
}

int get_delay(CellType type)
{
    switch (type)
    {
    case CellType::NOT:
        return 5;
    case CellType::AND:
        return 9;
    case CellType::OR:
        return 9;
    case CellType::XOR:
        return 12;
    case CellType::NAND:
        return 13;
    case CellType::NOR:
        return 12;
    case CellType::XNOR:
        return 12;
    case CellType::DFF_P:
        return -1; // Or any special value
    default:
        return 0;
    }
}

ASIC parse_json(const string &filename)
{
    ASIC asic;

    ifstream file(filename);
    json data;

    file >> data;

    int clock = -1;

    for (&auto top_name : data["modules"])
    {
        &auto cell_dict = top_name["cells"];

        vector<Cell> netlist;

        for (const auto &cell : cell_dict)
        {
            Cell new_cell;
            CellType type = parse_cell_type(cell["type"]);
            new_cell.type = type;
            new_cell.delay = get_delay(new_cell.type);

            vector<string> inputs;
            vector<string> outputs;

            for (&auto connection : cell["connections"])
            {
                if (cell["port_directions"][connection] == "input")
                {
                    for (&auto bit : cell["connections"][connection])
                    {
                        if (type != CellType::DFF_P || connection != "C")
                        {
                            inputs.push_back(bit);
                        }
                        else
                        {
                            clock = (int)bit;
                        }
                    }
                }
                else
                {
                    for (&auto bit : cell["connections"][connection])
                    {
                        outputs.append(bit);
                    }
                }
            }

            netlist.push_back(new_cell);
        }
        &auto port_dict = data["modules"][top_name]["ports"];

        vector<string> asic_inputs;
        vector<string> asic_outputs;

        for (&auto port : port_dict)
        {
            if (port["direction"] == "input")
            {
                for (&auto bit : port["bits"])
                {
                    if (int(bit) != clock)
                    {
                        asic_inputs.push_back(bit);
                    }
                }
            }
            else
            {
                for (&auto bit : port["bits"])
                {
                    asic_outputs.push_back(bit);
                }
            }
        }
    }
}