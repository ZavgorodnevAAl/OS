#include "logger.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

std::chrono::time_point<std::chrono::system_clock> programStartTime;
bool isProgramStartTimeSet = false;

time_t Logger::getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    time_t currentTime = std::chrono::system_clock::to_time_t(now);

#ifdef USE_SIMULATION
    if (!isProgramStartTimeSet) {
        programStartTime = now;
        isProgramStartTimeSet = true;
    }

    auto duration = now - programStartTime;
    double timeDiff = std::chrono::duration_cast<std::chrono::duration<double>>(duration).count();
    currentTime = static_cast<time_t>(std::chrono::system_clock::to_time_t(programStartTime) + timeDiff * simulationScale_);

#endif

    return currentTime;
}

Logger::Logger(const std::string &allReadingsLog, const std::string &hourlyAverageLog, const std::string &dailyAverageLog)
    : allReadingsLog_(allReadingsLog), hourlyAverageLog_(hourlyAverageLog), dailyAverageLog_(dailyAverageLog), simulationScale_(1) {
}

Logger::Logger(const std::string &allReadingsLog, const std::string &hourlyAverageLog, const std::string &dailyAverageLog, int scale)
    : allReadingsLog_(allReadingsLog), hourlyAverageLog_(hourlyAverageLog), dailyAverageLog_(dailyAverageLog), simulationScale_(scale) {
}

Logger::~Logger() {
    cleanupLogs();
}

void Logger::logTemperature(const std::string &temperature) {
    time_t currentTime = getCurrentTime();
    try {
        double tempValue = std::stod(temperature);
        temperatureReadings_.push_back(std::make_pair(currentTime, tempValue));

        std::stringstream ss;
        ss << std::put_time(localtime(&currentTime), "%Y-%m-%d %H:%M:%S") << " - " << temperature;
        writeLog(allReadingsLog_, ss.str(), true);
    } catch (std::invalid_argument &e) {
        std::cerr << "Error converting temperature to double" << e.what() << std::endl;
    } catch (std::out_of_range &e) {
        std::cerr << "Error converting temperature to double" << e.what() << std::endl;
    }
}

void Logger::updateLogs() {
    calculateHourlyAverage();
    calculateDailyAverage();
    cleanupLogs();
}

void Logger::writeLog(const std::string &fileName, const std::string &message, bool append) {
    std::ofstream logFile;
    if (append) {
        logFile.open(fileName, std::ios::app);
    } else {
        logFile.open(fileName);
    }

    if (logFile.is_open()) {
        logFile << message << std::endl;
        logFile.close();
    } else {
        std::cerr << "Unable to open log file: " << fileName << std::endl;
    }
}

void Logger::calculateHourlyAverage() {
    if (temperatureReadings_.empty()) {
        return;
    }

    time_t now = getCurrentTime();
    time_t currentHour = now - (now % 3600);

    double sum = 0.0;
    int count = 0;

    for (const auto &reading : temperatureReadings_) {
        if (reading.first >= currentHour) {
            sum += reading.second;
            count++;
        }
    }

    if (count > 0) {
        double average = sum / count;
        if (hourlyAverageReadings_.empty() || hourlyAverageReadings_.back().first != currentHour) {
            hourlyAverageReadings_.push_back(std::make_pair(currentHour, average));

            std::stringstream ss;
            ss << std::put_time(localtime(&currentHour), "%Y-%m-%d %H:%M:%S") << " - " << std::fixed << std::setprecision(2) << average;
            writeLog(hourlyAverageLog_, ss.str(), true);
        }
    }
}

void Logger::calculateDailyAverage() {
    if (temperatureReadings_.empty()) {
        return;
    }

    time_t now = getCurrentTime();
    time_t currentDay = now - (now % 86400);

    double sum = 0.0;
    int count = 0;

    for (const auto &reading : temperatureReadings_) {
        if (reading.first >= currentDay) {
            sum += reading.second;
            count++;
        }
    }

    if (count > 0) {
        double average = sum / count;

        if (dailyAverageReadings_.empty() || dailyAverageReadings_.back().first != currentDay) {
            dailyAverageReadings_.push_back(std::make_pair(currentDay, average));

            std::stringstream ss;
            ss << std::put_time(localtime(&currentDay), "%Y-%m-%d") << " - " << std::fixed << std::setprecision(2) << average;
            writeLog(dailyAverageLog_, ss.str(), true);
        }
    }
}

void Logger::cleanupLogs() {
    time_t now = getCurrentTime();
    time_t oneDayAgo = now - (24 * 3600);
    time_t oneMonthAgo = now - (30 * 24 * 3600);
    time_t oneYearAgo = now - (365 * 24 * 3600);

    std::vector<std::string> allReadings;
    std::ifstream allReadingsFile(allReadingsLog_);
    if (allReadingsFile.is_open()) {
        std::string line;
        while (getline(allReadingsFile, line)) {
            std::tm t{};
            std::istringstream ss(line);
            ss >> std::get_time(&t, "%Y-%m-%d %H:%M:%S");

            if (ss.fail()) {
                allReadings.push_back(line);
                continue;
            }

            time_t fileTime = mktime(&t);
            if (fileTime >= oneDayAgo) {
                allReadings.push_back(line);
            }
        }
        allReadingsFile.close();
    }

    std::ofstream allReadingsFileOut(allReadingsLog_);
    for (const auto &line : allReadings) {
        allReadingsFileOut << line << std::endl;
    }
    allReadingsFileOut.close();

    std::vector<std::string> hourlyAverages;
    std::ifstream hourlyAverageFile(hourlyAverageLog_);
    if (hourlyAverageFile.is_open()) {
        std::string line;
        while (getline(hourlyAverageFile, line)) {
            std::tm t{};
            std::istringstream ss(line);
            ss >> std::get_time(&t, "%Y-%m-%d %H:%M:%S");

            if (ss.fail()) {
                hourlyAverages.push_back(line);
                continue;
            }

            time_t fileTime = mktime(&t);
            if (fileTime >= oneMonthAgo) {
                hourlyAverages.push_back(line);
            }
        }
        hourlyAverageFile.close();
    }

    std::ofstream hourlyAverageFileOut(hourlyAverageLog_);
    for (const auto &line : hourlyAverages) {
        hourlyAverageFileOut << line << std::endl;
    }
    hourlyAverageFileOut.close();

    std::vector<std::string> dailyAverages;
    std::ifstream dailyAverageFile(dailyAverageLog_);
    if (dailyAverageFile.is_open()) {
        std::string line;
        while (getline(dailyAverageFile, line)) {
            std::tm t{};
            std::istringstream ss(line);
            ss >> std::get_time(&t, "%Y-%m-%d");
            if (ss.fail()) {
                dailyAverages.push_back(line);
                continue;
            }
            time_t fileTime = mktime(&t);
            if (fileTime >= oneYearAgo) {
                dailyAverages.push_back(line);
            }
        }
        dailyAverageFile.close();
    }

    std::ofstream dailyAverageFileOut(dailyAverageLog_);
    for (const auto &line : dailyAverages) {
        dailyAverageFileOut << line << std::endl;
    }
    dailyAverageFileOut.close();

    while (!temperatureReadings_.empty() && temperatureReadings_.front().first < oneDayAgo) {
        temperatureReadings_.pop_front();
    }

    while (!hourlyAverageReadings_.empty() && hourlyAverageReadings_.front().first < oneMonthAgo) {
        hourlyAverageReadings_.pop_front();
    }

    while (!dailyAverageReadings_.empty() && dailyAverageReadings_.front().first < oneYearAgo) {
        dailyAverageReadings_.pop_front();
    }
}
