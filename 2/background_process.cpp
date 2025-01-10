#include "background_process.h"
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <vector>

BackgroundProcess::BackgroundProcess() {
#if defined(_WIN32) || defined(_WIN64)
    memset(&processInfo, 0, sizeof(processInfo));
#else
    pid = 0;
#endif
}

BackgroundProcess::~BackgroundProcess() {
#if defined(_WIN32) || defined(_WIN64)
    if (processInfo.hProcess != NULL) {
      CloseHandle(processInfo.hProcess);
      CloseHandle(processInfo.hThread);
    }
#else
#endif
}

ProcessResult BackgroundProcess::execute(const std::string& command) {
    ProcessResult result;
    result.success = false;
    result.exitCode = -1;


#if defined(_WIN32) || defined(_WIN64)
    STARTUPINFOA startupInfo;
    memset(&startupInfo, 0, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);

    char *cmd_arr = new char[command.size()+1];
    strcpy(cmd_arr, command.c_str());
    
    BOOL creationResult = CreateProcessA(
        NULL,
        cmd_arr,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &startupInfo,
        &processInfo
    );

    delete [] cmd_arr;
    
    if (!creationResult) {
        std::cerr << "CreateProcess failed: " << GetLastError() << std::endl;
        return result;
    }

    WaitForSingleObject(processInfo.hProcess, INFINITE);

    DWORD exitCode;
    if (!GetExitCodeProcess(processInfo.hProcess, &exitCode)) {
       std::cerr << "GetExitCodeProcess failed: " << GetLastError() << std::endl;
       return result;
    }
    result.exitCode = exitCode;
    result.success = true;


#else
    pid = fork();

    if (pid == -1) {
        perror("fork failed");
        return result;
    } else if (pid == 0) {
        // Child process
        
        std::stringstream ss(command);
        std::vector<std::string> args;
        std::string arg;
        while (ss >> arg) {
            args.push_back(arg);
        }
            
        char** argv = new char*[args.size() + 1];
        for (size_t i = 0; i < args.size(); ++i) {
            argv[i] = strdup(args[i].c_str());
        }
        argv[args.size()] = nullptr;

        execvp(argv[0], argv);
        perror("execvp failed");
        exit(1);
    } else {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            result.exitCode = WEXITSTATUS(status);
            result.success = true;
        } else {
            result.exitCode = -1;
        }
    }
#endif
    return result;
}