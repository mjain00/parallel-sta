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
        return 0; // Or any special value
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
