#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#include "lua_bytecode_parser.h"

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 3) {
        std::cerr << "Usage: " << argv[0] << " <input_lua_bytecode_file.luac> [output_lua_bytecode_file.luac]" << std::endl;
        return 1;
    }

    std::string input_filename = argv[1];
    std::ifstream input_file(input_filename, std::ios::binary);

    if (!input_file.is_open()) {
        std::cerr << "Error: Could not open input file " << input_filename << std::endl;
        return 1;
    }

    
    std::vector<char> bytecode_data(
        (std::istreambuf_iterator<char>(input_file)),
        std::istreambuf_iterator<char>()
    );
    input_file.close();

    try {
        LuaBytecodeParser parser(bytecode_data);
        std::shared_ptr<LuaProto> main_proto = parser.parse();

        if (argc == 3) { 
            std::string output_filename = argv[2];
            std::ofstream output_file(output_filename, std::ios::binary);
            if (!output_file.is_open()) {
                std::cerr << "Error: Could not open output file " << output_filename << std::endl;
                return 1;
            }
            LuaBytecodeWriter writer(output_file);
            writer.write(*main_proto);
            output_file.close();
            std::cout << "Successfully parsed bytecode from " << input_filename
                      << " and wrote to " << output_filename << std::endl;
        } else { 
            LuaBytecodeFormatter formatter(std::cout);
            formatter.format(*main_proto);
            std::cout << "Successfully parsed and formatted bytecode from " << input_filename << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}