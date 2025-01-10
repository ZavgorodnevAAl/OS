#pragma once

#include <iostream>
#include <fstream>
#include <future>
#include <chrono>
#include <thread>
#include <mutex>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <csignal>
#include <queue>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif


std::int64_t loadCounterFromFile(const std::string& counterFilePath);
void saveCounterToFile(const std::string& counterFilePath, std::int64_t counter);
std::string generateTimestamp();
std::uint32_t getProcessId();
bool fileExists(const std::string& filename);
int lockFile(const std::string& filename);
void unlockFile(int fd);
bool isDataAvailable(std::istream& is);
void leaderProcess(int argc, char* argv[]);
void childProcess(int argc, char* argv[]);