#pragma once

#include <chrono>
#include <csignal>
#include <cstdint>
#include <fstream>
#include <future>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <sys/file.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

std::int64_t loadCounterFromFile(const std::string &counterFilePath);
void saveCounterToFile(const std::string &counterFilePath, std::int64_t counter);
std::string generateTimestamp();
std::uint32_t getProcessId();
bool fileExists(const std::string &filename);
int lockFile(const std::string &filename);
void unlockFile(int fd);
bool isDataAvailable(std::istream &is);
void leaderProcess(int argc, char *argv[]);
void childProcess(int argc, char *argv[]);