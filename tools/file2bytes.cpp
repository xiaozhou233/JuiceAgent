/*
 * File: file2bytes.cpp
 * Description: Converts any file to a C++ byte array header
*/

#include <fstream>
#include <iostream>
#include <vector>
#include <iomanip>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: file2bytes <input_file> <output.hpp> [var_name]\n";
        return 1;
    }

    const char* input = argv[1];
    const char* output = argv[2];
    std::string varName = (argc >= 4) ? argv[3] : "bytes";

    std::ifstream in(input, std::ios::binary | std::ios::ate);
    if (!in) {
        std::cerr << "Failed to open input file: " << input << "\n";
        return 1;
    }

    std::streamsize size = in.tellg();
    in.seekg(0, std::ios::beg);

    std::vector<unsigned char> data(size);
    if (!in.read(reinterpret_cast<char*>(data.data()), size)) {
        std::cerr << "Failed to read file\n";
        return 1;
    }

    std::ofstream out(output, std::ios::binary);
    if (!out) {
        std::cerr << "Failed to open output file: " << output << "\n";
        return 1;
    }

    out << "#pragma once\n\n";
    out << "#include <cstddef>\n\n";

    out << "static const unsigned char " << varName << "[] = {\n";

    for (size_t i = 0; i < data.size(); ++i) {
        if (i % 12 == 0) out << "    ";

        out << "0x"
            << std::hex
            << std::setw(2)
            << std::setfill('0')
            << static_cast<int>(data[i]);

        if (i + 1 != data.size())
            out << ", ";

        if (i % 12 == 11)
            out << "\n";
    }

    out << "\n};\n\n";
    out << "static const std::size_t " << varName << "_len = "
        << std::dec << data.size() << ";\n";

    std::cout << "Generated: " << output << "\n";
    return 0;
}