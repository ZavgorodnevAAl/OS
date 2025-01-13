#include "../include/logger.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <deque>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

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

Logger::Logger(const std::string &dbPath, int scale) : dbPath_(dbPath), simulationScale_(scale), db_(nullptr), insertStmt_(nullptr) {
    int rc = sqlite3_open(dbPath_.c_str(), &db_);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db_) << std::endl;
        return;
    }

    createTableIfNotExist();
    prepareStatements();
}

Logger::~Logger() {
    finalizeStatements();
    if (db_) {
        sqlite3_close(db_);
    }
    cleanupLogs();
}

void Logger::createTableIfNotExist() {
    if (!db_) {
        return;
    }

    const char *createTablesSQL =
        "CREATE TABLE IF NOT EXISTS all_readings ("
        "   time INTEGER NOT NULL,"
        "   temperature REAL NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS hourly_average ("
        "   time INTEGER NOT NULL,"
        "   average REAL NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS daily_average ("
        "   time INTEGER NOT NULL,"
        "   average REAL NOT NULL"
        ");";

    char *errMsg = nullptr;
    int rc = sqlite3_exec(db_, createTablesSQL, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
}

void Logger::prepareStatements() {
    if (!db_) {
        return;
    }

    const char *insertSQL = "INSERT INTO all_readings (time, temperature) VALUES (?, ?);";
    int rc = sqlite3_prepare_v2(db_, insertSQL, -1, &insertStmt_, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Error preparing insert statement: " << sqlite3_errmsg(db_) << std::endl;
        insertStmt_ = nullptr;
    }

    const char *insertHourlySQL = "INSERT INTO hourly_average (time, average) VALUES (?, ?);";
    rc = sqlite3_prepare_v2(db_, insertHourlySQL, -1, &insertHourlyStmt_, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Error preparing insert hourly average statement: " << sqlite3_errmsg(db_) << std::endl;
        insertHourlyStmt_ = nullptr;
    }

    const char *insertDailySQL = "INSERT INTO daily_average (time, average) VALUES (?, ?);";
    rc = sqlite3_prepare_v2(db_, insertDailySQL, -1, &insertDailyStmt_, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Error preparing insert daily average statement: " << sqlite3_errmsg(db_) << std::endl;
        insertDailyStmt_ = nullptr;
    }
}

void Logger::finalizeStatements() {
    if (insertStmt_) {
        sqlite3_finalize(insertStmt_);
        insertStmt_ = nullptr;
    }

    if (insertHourlyStmt_) {
        sqlite3_finalize(insertHourlyStmt_);
        insertHourlyStmt_ = nullptr;
    }

    if (insertDailyStmt_) {
        sqlite3_finalize(insertDailyStmt_);
        insertDailyStmt_ = nullptr;
    }
}

void Logger::insertReading(time_t time, double temp) {
    if (!db_ || !insertStmt_) {
        return;
    }

    sqlite3_reset(insertStmt_);
    sqlite3_bind_int64(insertStmt_, 1, time);
    sqlite3_bind_double(insertStmt_, 2, temp);

    int rc = sqlite3_step(insertStmt_);
    if (rc != SQLITE_DONE) {
        std::cerr << "SQL error during insert: " << sqlite3_errmsg(db_) << std::endl;
    }
}

void Logger::insertAverage(time_t time, double average, const std::string &table) {
    sqlite3_stmt *stmt = nullptr;
    if (table == "hourly_average") {
        stmt = insertHourlyStmt_;
    } else if (table == "daily_average") {
        stmt = insertDailyStmt_;
    } else {
        return;
    }

    if (!db_ || !stmt) {
        return;
    }

    sqlite3_reset(stmt);
    sqlite3_bind_int64(stmt, 1, time);
    sqlite3_bind_double(stmt, 2, average);

    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "SQL error during insert average: " << sqlite3_errmsg(db_) << std::endl;
    }
}

void Logger::logTemperature(const std::string &temperature) {
    time_t currentTime = getCurrentTime();
    try {
        double tempValue = std::stod(temperature);
        temperatureReadings_.push_back(std::make_pair(currentTime, tempValue));
        insertReading(currentTime, tempValue);

    } catch (std::invalid_argument &e) {
        std::cerr << "Error converting temperature to double: " << e.what() << std::endl;
    } catch (std::out_of_range &e) {
        std::cerr << "Error converting temperature to double: " << e.what() << std::endl;
    }
}

void Logger::updateLogs() {
    calculateHourlyAverage();
    calculateDailyAverage();
    cleanupLogs();
    cleanupDatabase();
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
            insertAverage(currentHour, average, "hourly_average");
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
            insertAverage(currentDay, average, "daily_average");
        }
    }
}

void Logger::cleanupLogs() {
    time_t now = getCurrentTime();
    time_t oneDayAgo = now - (24 * 3600);
    time_t oneMonthAgo = now - (30 * 24 * 3600);
    time_t oneYearAgo = now - (365 * 24 * 3600);

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

void Logger::cleanupDatabase() {
    if (!db_) {
        return;
    }

    time_t now = getCurrentTime();
    time_t oneDayAgo = now - (24 * 3600);
    time_t oneMonthAgo = now - (30 * 24 * 3600);
    time_t oneYearAgo = now - (365 * 24 * 3600);

    char *errMsg = nullptr;
    int rc;

    // Delete from all_readings
    std::string deleteReadingsSQL = "DELETE FROM all_readings WHERE time < ?";
    sqlite3_stmt *deleteReadingsStmt;
    rc = sqlite3_prepare_v2(db_, deleteReadingsSQL.c_str(), -1, &deleteReadingsStmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Error preparing delete readings statement: " << sqlite3_errmsg(db_) << std::endl;
        return;
    }
    sqlite3_bind_int64(deleteReadingsStmt, 1, oneDayAgo);
    rc = sqlite3_step(deleteReadingsStmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Error deleting from all_readings: " << sqlite3_errmsg(db_) << std::endl;
    }
    sqlite3_finalize(deleteReadingsStmt);

    // Delete from hourly_average
    std::string deleteHourlySQL = "DELETE FROM hourly_average WHERE time < ?";
    sqlite3_stmt *deleteHourlyStmt;
    rc = sqlite3_prepare_v2(db_, deleteHourlySQL.c_str(), -1, &deleteHourlyStmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Error preparing delete hourly statement: " << sqlite3_errmsg(db_) << std::endl;
        return;
    }
    sqlite3_bind_int64(deleteHourlyStmt, 1, oneMonthAgo);
    rc = sqlite3_step(deleteHourlyStmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Error deleting from hourly_average: " << sqlite3_errmsg(db_) << std::endl;
    }
    sqlite3_finalize(deleteHourlyStmt);

    // Delete from daily_average
    std::string deleteDailySQL = "DELETE FROM daily_average WHERE time < ?";
    sqlite3_stmt *deleteDailyStmt;
    rc = sqlite3_prepare_v2(db_, deleteDailySQL.c_str(), -1, &deleteDailyStmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Error preparing delete daily statement: " << sqlite3_errmsg(db_) << std::endl;
        return;
    }
    sqlite3_bind_int64(deleteDailyStmt, 1, oneYearAgo);
    rc = sqlite3_step(deleteDailyStmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Error deleting from daily_average: " << sqlite3_errmsg(db_) << std::endl;
    }
    sqlite3_finalize(deleteDailyStmt);
}

std::vector<std::pair<time_t, double>> Logger::getAllReadings() {
    std::vector<std::pair<time_t, double>> readings;
    if (!db_) {
        return readings;
    }

    const char *sql = "SELECT time, temperature FROM all_readings;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare SQL: " << sqlite3_errmsg(db_) << std::endl;
        return readings;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        time_t time = sqlite3_column_int64(stmt, 0);
        double temp = sqlite3_column_double(stmt, 1);
        readings.push_back(std::make_pair(time, temp));
    }

    if (rc != SQLITE_DONE) {
        std::cerr << "Error during SQL query: " << sqlite3_errmsg(db_) << std::endl;
    }

    sqlite3_finalize(stmt);
    return readings;
}

std::vector<std::pair<time_t, double>> Logger::getHourlyAverageReadings() {
    std::vector<std::pair<time_t, double>> readings;
    if (!db_) {
        return readings;
    }

    const char *sql = "SELECT time, average FROM hourly_average;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare SQL: " << sqlite3_errmsg(db_) << std::endl;
        return readings;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        time_t time = sqlite3_column_int64(stmt, 0);
        double average = sqlite3_column_double(stmt, 1);
        readings.push_back(std::make_pair(time, average));
    }

    if (rc != SQLITE_DONE) {
        std::cerr << "Error during SQL query: " << sqlite3_errmsg(db_) << std::endl;
    }

    sqlite3_finalize(stmt);
    return readings;
}

std::vector<std::pair<time_t, double>> Logger::getDailyAverageReadings() {
    std::vector<std::pair<time_t, double>> readings;
    if (!db_) {
        return readings;
    }

    const char *sql = "SELECT time, average FROM daily_average;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare SQL: " << sqlite3_errmsg(db_) << std::endl;
        return readings;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        time_t time = sqlite3_column_int64(stmt, 0);
        double average = sqlite3_column_double(stmt, 1);
        readings.push_back(std::make_pair(time, average));
    }

    if (rc != SQLITE_DONE) {
        std::cerr << "Error during SQL query: " << sqlite3_errmsg(db_) << std::endl;
    }

    sqlite3_finalize(stmt);
    return readings;
}
