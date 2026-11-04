#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "usage: child <file name>" << std::endl;
        return 1;
    }

    std::ofstream file(argv[1]);
    if (!file.is_open()) {
        std::cerr << "child: cannot open file " << argv[1] << std::endl;
        return 1;
    }

    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream stream(line);
        float number = 0;
        float sum = 0;
        int count = 0;

        while (stream >> number) {
            sum += number;
            count++;
        }

        if (!stream.eof()) {
            std::cout << "error: line contains not a number" << std::endl;
            continue;
        }
        if (count == 0) {
            std::cout << "error: empty line" << std::endl;
            continue;
        }

        file << sum << std::endl;
        if (!file) {
            std::cerr << "child: cannot write to file" << std::endl;
            return 1;
        }

        std::cout << "sum = " << sum << std::endl;
    }

    return 0;
}
