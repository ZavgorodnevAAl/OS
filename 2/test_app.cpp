#include "background_process.h"
#include <iostream>

int main() {
    BackgroundProcess process;

    std::cout << "Test 1: Running 'echo Hello from background'..." << std::endl;
    ProcessResult result1 = process.execute("echo Hello from background");
    if (result1.success) {
        std::cout << "  Success, exit code: " << result1.exitCode << std::endl;
    } else {
        std::cout << "  Failed" << std::endl;
    }

    std::cout << "Test 2: Running 'ls -l' or 'dir'..." << std::endl;
    #if defined(_WIN32) || defined(_WIN64)
        ProcessResult result2 = process.execute("dir");
    #else
        ProcessResult result2 = process.execute("ls -l");
    #endif
    
    if (result2.success) {
        std::cout << "  Success, exit code: " << result2.exitCode << std::endl;
    } else {
        std::cout << "  Failed" << std::endl;
    }

    std::cout << "Test 3: Running non-existent command..." << std::endl;
    ProcessResult result3 = process.execute("nonexistentcommand");
     if (result3.success) {
        std::cout << "  Success, exit code: " << result3.exitCode << std::endl;
    } else {
        std::cout << "  Failed" << std::endl;
    }

    return 0;
}