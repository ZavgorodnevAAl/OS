#include "counter.h"
#include <fstream>

void saveCounterToFile(const std::string& counterFilePath, std::int64_t counter) {
    std::ofstream counterFile(counterFilePath);
    if (counterFile.is_open()) {
        counterFile << counter;
    }
}

std::string generateTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
    auto value = now_ms.time_since_epoch().count();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm;
#ifdef _WIN32
    localtime_s(&now_tm, &now_c);
#else
    localtime_r(&now_c, &now_tm);
#endif
    std::stringstream ss;
    ss << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S") << "." << std::setfill('0') << std::setw(3) << value % 1000;
    return ss.str();
}

std::uint32_t getProcessId() {
#ifdef _WIN32
    return GetCurrentProcessId();
#else
    return getpid();
#endif
}

bool fileExists(const std::string& filename) {
#ifdef _WIN32
    return _access(filename.c_str(), 0) != -1;
#else
    return access(filename.c_str(), F_OK) != -1;
#endif
}

int lockFile(const std::string& filename) {
#ifdef _WIN32
    HANDLE hFile = CreateFileA(filename.c_str(), GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return -1;

    OVERLAPPED ov = { 0 };
    if (!LockFileEx(hFile, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, MAXDWORD, MAXDWORD, &ov)) {
        CloseHandle(hFile);
        return -1;
    }
    
    return 1;
#else
    int fd = open(filename.c_str(), O_RDWR | O_CREAT, 0666);
    if (fd < 0) return -1;

    if (flock(fd, LOCK_EX | LOCK_NB) < 0) {
        close(fd);
        return -1;
    }
    
    return 1;
#endif
}

void unlockFile(int fd) {
#ifdef _WIN32
    if (fd != -1) {
        HANDLE hFile = reinterpret_cast<HANDLE>(fd);
        OVERLAPPED ov = { 0 };
        UnlockFileEx(hFile, 0, MAXDWORD, MAXDWORD, &ov);
        CloseHandle(hFile);
    }
#else
    if (fd != -1) {
        flock(fd, LOCK_UN);
        close(fd);
    }
#endif
}

std::int64_t loadCounterFromFile(const std::string& counterFilePath) {
    std::int64_t counter = 32;

#ifdef _WIN32
    HANDLE hFile = CreateFileA(counterFilePath.c_str(), GENERIC_READ, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return counter;
    }

    OVERLAPPED ov = { 0 };
    if (!LockFileEx(hFile, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &ov)) {
        CloseHandle(hFile);
        return counter;
    }

    int fd = _open_osfhandle(reinterpret_cast<intptr_t>(hFile), _O_RDONLY);
    if (fd == -1) {
        CloseHandle(hFile);
        return counter;
    }

    std::vector<char> buffer(128);
    int bytesRead = _read(fd, buffer.data(), buffer.size());

    _close(fd);
    CloseHandle(hFile);
    if (bytesRead > 0) {
        std::stringstream ss(std::string(buffer.begin(), buffer.begin() + bytesRead));
        ss >> counter;
    }


#else
    int fd = open(counterFilePath.c_str(), O_RDONLY | O_CREAT, 0666);
    if (fd < 0) {
        return counter;
    }

    if (flock(fd, LOCK_EX) < 0) {
        close(fd);
        return counter;
    }
    std::vector<char> buffer(128);
    int bytesRead = read(fd, buffer.data(), buffer.size());

    close(fd);
    if (bytesRead > 0) {
       std::stringstream ss(std::string(buffer.begin(), buffer.begin() + bytesRead));
        ss >> counter;
    }
#endif

    return counter;
}


bool isDataAvailable(std::istream& is) {
#ifdef _WIN32
    HANDLE hFile = (HANDLE)_get_osfhandle(_fileno(stdin));
    DWORD bytesAvailable = 0;
    if (!PeekNamedPipe(hFile, NULL, 0, NULL, &bytesAvailable, NULL)) {
        return false;
    }
    return bytesAvailable > 0;
#else
    fd_set readFds;
    FD_ZERO(&readFds);
    FD_SET(STDIN_FILENO, &readFds);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    int result = select(STDIN_FILENO + 1, &readFds, NULL, NULL, &timeout);
    return result > 0 && FD_ISSET(STDIN_FILENO, &readFds);
#endif
}

void leaderProcess(int argc, char* argv[]) {
    std::string logFilePath = argv[1];
    std::string lockFilePath = logFilePath + ".leader";
    std::string counterFilePath = logFilePath + ".counter";

    int lockFileDescriptor = lockFile(lockFilePath);
    bool isLeader = lockFileDescriptor != -1;

    std::ofstream logFile;
    logFile.open(logFilePath, std::ios::app);

    std::uint32_t pid = getProcessId();

    logFile << generateTimestamp() << " - Process " << pid << " started." << std::endl;

    bool child1Running = false;
    bool child2Running = false;
    std::mutex childMutex;

    if (isLeader) {
        std::thread counterThread([&]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            {
                int64_t counter = loadCounterFromFile(counterFilePath);
                saveCounterToFile(counterFilePath, ++counter);
            }
        }
        });
        counterThread.detach();
    }

    auto lastLogTime = std::chrono::system_clock::now();
    auto lastSpawnTime = std::chrono::system_clock::now();

    while (true) {        
        if (!isLeader) {
            int currentLockFileDescriptor = lockFile(lockFilePath);
            bool currentIsLeader = currentLockFileDescriptor != -1;
            if (isLeader != currentIsLeader) {
                isLeader = currentIsLeader;
                if (!isLeader) {
                    logFile << generateTimestamp() << " - Process " << pid << " lost leader status." << std::endl;
                } else {
                    logFile << generateTimestamp() << " - Process " << pid << " become leader." << std::endl;

                    std::thread counterThread([&]() {
                    while (true) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(300));
                        {
                            int64_t counter = loadCounterFromFile(counterFilePath);
                            saveCounterToFile(counterFilePath, ++counter);
                        }
                    }
                    });
                    counterThread.detach();
                }
            }
        }

        if (isDataAvailable(std::cin)) {
            std::string input;
            if(std::getline(std::cin, input)){
                if (input == "EXIT") {
                    break;
                }
                try {
                    std::int64_t newCounterValue = std::stoll(input);
                    {
                        saveCounterToFile(counterFilePath, newCounterValue);
                    }
                    logFile << generateTimestamp() << " - Process " << pid << " set counter to: " << newCounterValue << std::endl;
                } catch (const std::exception& e) {
                    logFile << generateTimestamp() << " - Process " << pid << " error, invalid input: " << input << std::endl;
                }
            }
        }
        if (isLeader) {
            auto now = std::chrono::system_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - lastLogTime).count() >= 1) {
                {
                    int64_t counter = loadCounterFromFile(counterFilePath);
                    logFile << generateTimestamp() << " - Process " << pid << ", Counter: " << counter << std::endl;
                }
                lastLogTime = now;
            }

            if (std::chrono::duration_cast<std::chrono::seconds>(now - lastSpawnTime).count() >= 3) {
                std::lock_guard<std::mutex> childLock(childMutex);

                if (!child1Running) {
                    child1Running = true;
                #ifdef _WIN32
                    STARTUPINFOA si1 = { sizeof(si1) };
                    PROCESS_INFORMATION pi1;
                    std::string commandLine1 = "\"" + std::string(argv[0]) + "\" \"" + logFilePath + "\" child1";
                    if (CreateProcessA(NULL, (char*)commandLine1.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si1, &pi1)) {
                        CloseHandle(pi1.hThread);
                        std::thread([&, processHandle = pi1.hProcess]() {
                            WaitForSingleObject(processHandle, INFINITE);
                            CloseHandle(processHandle);
                            {
                                std::lock_guard<std::mutex> childLockInner(childMutex);
                                child1Running = false;
                            }
                            }).detach();
                    } else {
                        logFile << generateTimestamp() << " - Process " << pid << " failed to start child1." << std::endl;
                        child1Running = false;
                    }
                #else
                    pid_t childPid1 = fork();
                    if (childPid1 == 0) {
                        std::string commandLine1 = std::string(argv[0]) + " " + logFilePath + " child1";
                        execl(argv[0], argv[0], logFilePath.c_str(), "child1", nullptr);
                        logFile << generateTimestamp() << " - Process " << getProcessId() << " failed to start child1." << std::endl;
                        exit(1);
                    } else if (childPid1 > 0) {
                        std::thread([&, childPid = childPid1]() {
                            int status;
                            waitpid(childPid, &status, 0);
                            {
                            std::lock_guard<std::mutex> childLockInner(childMutex);
                                child1Running = false;
                            }
                        }).detach();
                    } else {
                        logFile << generateTimestamp() << " - Process " << pid << " failed to start child1." << std::endl;
                        child1Running = false;
                    }
                #endif
                } else {
                    logFile << generateTimestamp() << " - Process " << pid << " child1 is still running, not spawning a new instance." << std::endl;
                }

                if (!child2Running) {
                child2Running = true;
                #ifdef _WIN32
                    STARTUPINFOA si2 = { sizeof(si2) };
                    PROCESS_INFORMATION pi2;
                    std::string commandLine2 = "\"" + std::string(argv[0]) + "\" \"" + logFilePath + "\" child2";
                    if (CreateProcessA(NULL, (char*)commandLine2.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si2, &pi2)) {
                    CloseHandle(pi2.hThread);
                    std::thread([&, processHandle = pi2.hProcess]() {
                        WaitForSingleObject(processHandle, INFINITE);
                        CloseHandle(processHandle);
                        {
                            std::lock_guard<std::mutex> childLockInner(childMutex);
                            child2Running = false;
                        }
                        }).detach();
                    } else {
                        logFile << generateTimestamp() << " - Process " << pid << " failed to start child2." << std::endl;
                        child2Running = false;
                    }
                #else
                    pid_t childPid2 = fork();
                    if (childPid2 == 0) {
                        std::string commandLine2 = std::string(argv[0]) + " " + logFilePath + " child2";
                        execl(argv[0], argv[0], logFilePath.c_str(), "child2", nullptr);
                        logFile << generateTimestamp() << " - Process " << getProcessId() << " failed to start child2." << std::endl;
                        exit(1);
                    } else if (childPid2 > 0) {
                    std::thread([&, childPid = childPid2]() {
                        int status;
                        waitpid(childPid, &status, 0);
                        {
                            std::lock_guard<std::mutex> childLockInner(childMutex);
                            child2Running = false;
                        }
                        }).detach();
                    } else {
                        logFile << generateTimestamp() << " - Process " << pid << " failed to start child2." << std::endl;
                        child2Running = false;
                    }
                #endif
            } else {
                logFile << generateTimestamp() << " - Process " << pid << " child2 is still running, not spawning a new instance." << std::endl;
            }
            lastSpawnTime = now;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (std::cin.eof()) break;
    }
    logFile << generateTimestamp() << " - Process " << pid << " exited." << std::endl;

    logFile.close();
    unlockFile(lockFileDescriptor);
}

void childProcess(int argc, char* argv[]) {
    std::string name = argv[2];
    std::string logFilePath = argv[1];
    std::string lockFilePath = logFilePath + ".lock";
    std::string counterFilePath = logFilePath + ".counter";

    std::uint32_t pid = getProcessId();

    std::ofstream logFile;
    logFile.open(logFilePath, std::ios::app);

    auto updateValue = [&](int64_t (*update_fun)(int64_t )) {
        int64_t value;
        {
            std::int64_t counter = loadCounterFromFile(counterFilePath);
            value = update_fun(counter);
            saveCounterToFile(counterFilePath, value);
        }
        logFile << generateTimestamp() << " - Process " << pid << " (" << name << ") set counter to: " << value << std::endl;
    };

    if (name == "child1") {
        updateValue([](int64_t v) { return v + 10; });
    } else {
        updateValue([](int64_t v) { return v * 2; });
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        updateValue([](int64_t v) { return v / 2; });
    }
}