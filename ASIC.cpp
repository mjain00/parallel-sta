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

    cout << "\n\nCell List:" << endl;

    for (const auto &cell : asic.cells)
    {
        cout << "Cell ID: " << cell.id << endl;
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
        cout << endl << endl;
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
    cout << endl << endl;

    cout << "Paths: ";
    for (const auto &path : asic.paths)
    {
        cout << path << " ";
    }
    cout << endl << endl;

    cout << "Net Mappings:" << endl;
    for (const auto &pair : asic.net_dict)
    {
        cout << "[" << pair.first << "] -> " << pair.second << endl;
    }
}

map<int, Cell> create_cell_map(const vector<Cell>& cells) {
    map<int, Cell> cell_map;
    for (const auto& cell : cells) {
        cell_map[cell.id] = cell;
    }
    return cell_map;
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
    if (type_str == "$_DFF_PN0_")
        return CellType::DFF_PN0;
    if (type_str == "$_MUX_")
        return CellType::MUX;
    if (type_str == "AND2_X2")
        return CellType::AND2_X2;
    if (type_str == "AND2_X4")
        return CellType::AND2_X4;
    if (type_str == "AND3_X2")
        return CellType::AND3_X2;
    if (type_str == "AND3_X4")
        return CellType::AND3_X4;
    if (type_str == "AND4_X2")
        return CellType::AND4_X2;
    if (type_str == "AND4_X4")
        return CellType::AND4_X4;
    if (type_str == "AOI211_X2")
        return CellType::AOI211_X2;
    if (type_str == "AOI21_X2")
        return CellType::AOI21_X2;
    if (type_str == "AOI21_X4")
        return CellType::AOI21_X4;
    if (type_str == "AOI221_X2")
        return CellType::AOI221_X2;
    if (type_str == "AOI222_X1")
        return CellType::AOI222_X1;
    if (type_str == "AOI222_X2")
        return CellType::AOI222_X2;
    if (type_str == "AOI22_X2")
        return CellType::AOI22_X2;
    if (type_str == "CLKBUF_X1")
        return CellType::CLKBUF_X1;
    if (type_str == "CLKBUF_X2")
        return CellType::CLKBUF_X2;
    if (type_str == "DFFR_X1")
        return CellType::DFFR_X1;
    if (type_str == "DFFR_X2")
        return CellType::DFFR_X2;
    if (type_str == "DFFS_X2")
        return CellType::DFFS_X2;
    if (type_str == "DFF_X1")
        return CellType::DFF_X1;
    if (type_str == "DFF_X2")
        return CellType::DFF_X2;
    if (type_str == "HA_X1")
        return CellType::HA_X1;
    if (type_str == "INV_X1")
        return CellType::INV_X1;
    if (type_str == "INV_X16")
        return CellType::INV_X16;
    if (type_str == "INV_X2")
        return CellType::INV_X2;
    if (type_str == "INV_X32")
        return CellType::INV_X32;
    if (type_str == "INV_X4")
        return CellType::INV_X4;
    if (type_str == "INV_X8")
        return CellType::INV_X8;
    if (type_str == "MUX2_X1")
        return CellType::MUX2_X1;
    if (type_str == "NAND2_X1")
        return CellType::NAND2_X1;
    if (type_str == "NAND2_X2")
        return CellType::NAND2_X2;
    if (type_str == "NAND2_X4")
        return CellType::NAND2_X4;
    if (type_str == "NAND3_X2")
        return CellType::NAND3_X2;
    if (type_str == "NAND3_X4")
        return CellType::NAND3_X4;
    if (type_str == "NAND4_X2")
        return CellType::NAND4_X2;
    if (type_str == "NOR2_X2")
        return CellType::NOR2_X2;
    if (type_str == "NOR2_X4")
        return CellType::NOR2_X4;
    if (type_str == "NOR3_X2")
        return CellType::NOR3_X2;
    if (type_str == "NOR3_X4")
        return CellType::NOR3_X4;
    if (type_str == "NOR4_X2")
        return CellType::NOR4_X2;
    if (type_str == "OAI211_X2")
        return CellType::OAI211_X2;
    if (type_str == "OAI21_X2")
        return CellType::OAI21_X2;
    if (type_str == "OAI221_X2")
        return CellType::OAI221_X2;
    if (type_str == "OAI222_X2")
        return CellType::OAI222_X2;
    if (type_str == "OAI22_X1")
        return CellType::OAI22_X1;
    if (type_str == "OAI22_X2")
        return CellType::OAI22_X2;
    if (type_str == "OR2_X2")
        return CellType::OR2_X2;
    if (type_str == "OR2_X4")
        return CellType::OR2_X4;
    if (type_str == "SDFFR_X2")
        return CellType::SDFFR_X2;
    if (type_str == "SDFF_X2")
        return CellType::SDFF_X2;
    if (type_str == "XNOR2_X2")
        return CellType::XNOR2_X2;
    if (type_str == "XOR2_X2")
        return CellType::XOR2_X2;
    return CellType::UNKNOWN;
}

int get_delay(CellType type)
{
    switch (type)
    {
    case CellType::NOT: return 5;
    case CellType::AND: return 9;
    case CellType::OR: return 9;
    case CellType::XOR: return 12;
    case CellType::NAND: return 13;
    case CellType::NOR: return 12;
    case CellType::XNOR: return 12;

    case CellType::DFF_P:
    case CellType::DFF_PN0:
    case CellType::DFFR_X1:
    case CellType::DFFR_X2:
    case CellType::DFFS_X2:
    case CellType::DFF_X1:
    case CellType::DFF_X2:
    case CellType::SDFFR_X2:
    case CellType::SDFF_X2:
        return -1;

    case CellType::MUX:
    case CellType::MUX2_X1: return 14;

    case CellType::HA_X1: return 15;

    case CellType::INV_X1: return 5;
    case CellType::INV_X2: return 4;
    case CellType::INV_X4: return 3;
    case CellType::INV_X8: return 2;
    case CellType::INV_X16: return 2;
    case CellType::INV_X32: return 1;

    case CellType::AND2_X2: return 8;
    case CellType::AND2_X4: return 7;
    case CellType::AND3_X2: return 10;
    case CellType::AND3_X4: return 9;
    case CellType::AND4_X2: return 12;
    case CellType::AND4_X4: return 11;

    case CellType::NAND2_X1: return 10;
    case CellType::NAND2_X2: return 9;
    case CellType::NAND2_X4: return 8;
    case CellType::NAND3_X2: return 11;
    case CellType::NAND3_X4: return 10;
    case CellType::NAND4_X2: return 13;

    case CellType::OR2_X2: return 8;
    case CellType::OR2_X4: return 7;

    case CellType::NOR2_X2: return 9;
    case CellType::NOR2_X4: return 8;
    case CellType::NOR3_X2: return 10;
    case CellType::NOR3_X4: return 9;
    case CellType::NOR4_X2: return 12;

    case CellType::AOI211_X2: return 13;
    case CellType::AOI21_X2: return 12;
    case CellType::AOI21_X4: return 11;
    case CellType::AOI221_X2: return 14;
    case CellType::AOI222_X1: return 14;
    case CellType::AOI222_X2: return 13;
    case CellType::AOI22_X2: return 13;

    case CellType::CLKBUF_X1: return 3;
    case CellType::CLKBUF_X2: return 2;

    case CellType::OAI211_X2: return 13;
    case CellType::OAI21_X2: return 12;
    case CellType::OAI221_X2: return 14;
    case CellType::OAI222_X2: return 14;
    case CellType::OAI22_X1: return 13;
    case CellType::OAI22_X2: return 12;

    case CellType::XNOR2_X2: return 12;
    case CellType::XOR2_X2: return 12;

    default:
        return 0;
    }
}

ASIC parse_json(const string &filename)
{
    ASIC asic;

    ifstream file(filename);
    json data;

    try {
        file >> data;
    } catch (const std::exception &e) {
        std::cerr << "Error parsing JSON file: " << e.what() << std::endl;
        return asic;
    }

    int clock = -1;

    try {
        for (auto &[top_name, module_data] : data["modules"].items())
        {
            auto &cell_dict = module_data["cells"];
            vector<Cell> netlist;

            for (const auto &cell : cell_dict)
            {
                Cell new_cell;
                CellType type = parse_cell_type(cell["type"]);
                new_cell.type = type;
                new_cell.delay = get_delay(new_cell.type);

                vector<int> output_bits;

                for (auto &[connection, bits] : cell["connections"].items())
                {
                    const auto &direction = cell["port_directions"][connection];
                    if (!direction.is_string()) {
                        std::cerr << "Direction at connection '" << connection << "' is not a string!" << std::endl;
                        continue;
                    }

                    if (direction == "input")
                    {
                        for (auto &bit : bits)
                        {
                            if (!bit.is_number()) {
                                std::cerr << "Expected number in input bits but got: " << bit << std::endl;
                                continue;
                            }

                            int bit_val = bit.get<int>();
                            if (type != CellType::DFF_P || connection != "C")
                            {
                                new_cell.inputs.push_back(bit_val);
                            }
                            else
                            {
                                clock = bit_val;
                            }
                        }
                    }
                    else // output
                    {
                        for (auto &bit : bits)
                        {
                            if (!bit.is_number()) {
                                std::cerr << "Expected number in output bits but got: " << bit << std::endl;
                                continue;
                            }

                            int bit_val = bit.get<int>();
                            new_cell.outputs.push_back(bit_val);
                            output_bits.push_back(bit_val);
                        }
                    }
                }

                // Set cell.id to the output bit number (use first one if multiple)
                if (!output_bits.empty())
                {
                    new_cell.id = output_bits[0];
                }
                else
                {
                    new_cell.id = -1;
                }

                asic.cells.push_back(new_cell);
            }

            auto &port_dict = data["modules"][top_name]["ports"];
            for (auto &[port_name, port_details] : port_dict.items())
            {
                const auto &direction = port_details["direction"];
                if (!direction.is_string()) {
                    std::cerr << "Port direction for " << port_name << " is not a string" << std::endl;
                    continue;
                }

                for (auto &bit : port_details["bits"])
                {
                    if (!bit.is_number()) {
                        std::cerr << "Expected number in port bits but got: " << bit << std::endl;
                        continue;
                    }

                    int bit_val = bit.get<int>();
                    if (direction == "input")
                    {
                        if (bit_val != clock)
                        {
                            asic.inputs.push_back(bit_val);
                        }
                    }
                    else
                    {
                        asic.outputs.push_back(bit_val);
                    }
                }
            }

            auto &net_names = data["modules"][top_name]["netnames"];
            for (auto &[net_name, net_info] : net_names.items())
            {
                auto &bit_list = net_info["bits"];
                for (int i = 0; i < bit_list.size(); i++)
                {
                    if (!bit_list[i].is_number()) {
                        std::cerr << "Netname bit is not a number: " << bit_list[i] << std::endl;
                        continue;
                    }

                    if (bit_list.size() == 1)
                    {
                        asic.net_dict[bit_list[i]] = net_name;
                    }
                    else
                    {
                        asic.net_dict[bit_list[i]] = net_name + "[" + to_string(i) + "]";
                    }
                }
            }
        }
    } catch (const nlohmann::json::type_error &e) {
        std::cerr << "JSON type error: " << e.what() << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Exception while parsing JSON: " << e.what() << std::endl;
    }

    return asic;
}
