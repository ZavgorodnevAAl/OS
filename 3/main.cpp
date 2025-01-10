#include "counter.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "specify the path to the file" << std::endl;
        return 1;
    }

    if (argc == 3) {
        childProcess(argc, argv);
        return 0;
    }

    std::string logFilePath = argv[1];
    leaderProcess(argc, argv);
    return 0;
}
