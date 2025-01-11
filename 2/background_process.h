#ifndef BACKGROUND_PROCESS_H
#define BACKGROUND_PROCESS_H

#include <string>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

struct ProcessResult {
    int exitCode;
    bool success;
};

class BackgroundProcess {
public:
    BackgroundProcess();
    ~BackgroundProcess();

    ProcessResult execute(const std::string &command);

private:
#if defined(_WIN32) || defined(_WIN64)
    PROCESS_INFORMATION processInfo;
#else
    pid_t pid;
#endif
};

#endif