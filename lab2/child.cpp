// Дочерний процесс.
// stdin  - связан с pipe1 (сюда родитель присылает строки с числами)
// stdout - связан с pipe2 (сюда отправляем ответ родителю)
// Сумму чисел каждой строки записываем в файл, имя которого пришло в argv[1].

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "usage: child <file name>" << std::endl;
        return 1;
    }

    // создаём (или очищаем) файл для результатов
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

        // если чтение остановилось не в конце строки - там было не число
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

        // std::endl сбрасывает буфер, поэтому родитель сразу получит ответ
        std::cout << "sum = " << sum << std::endl;
    }

    return 0;
}
