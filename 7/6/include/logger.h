#include "sqlite3.h"
#include <ctime>
#include <deque>
#include <string>
#include <vector>

class Logger {
public:
    Logger(const std::string &dbPath, int scale = 1);
    ~Logger();

    void logTemperature(const std::string &temperature);
    void updateLogs();
    void writeLog(const std::string &fileName, const std::string &message, bool append = true);

    std::vector<std::pair<time_t, double>> getAllReadings();
    std::vector<std::pair<time_t, double>> getHourlyAverageReadings();
    std::vector<std::pair<time_t, double>> getDailyAverageReadings();

private:
    time_t getCurrentTime();
    void createTableIfNotExist();
    void prepareStatements();
    void finalizeStatements();
    void insertReading(time_t time, double temp);
    void insertAverage(time_t time, double average, const std::string &table);

    void calculateHourlyAverage();
    void calculateDailyAverage();
    void cleanupLogs();
    void cleanupDatabase();

    std::string dbPath_;
    int simulationScale_;
    sqlite3 *db_;
    sqlite3_stmt *insertStmt_;
    sqlite3_stmt *insertHourlyStmt_;
    sqlite3_stmt *insertDailyStmt_;

    std::deque<std::pair<time_t, double>> temperatureReadings_;
    std::deque<std::pair<time_t, double>> hourlyAverageReadings_;
    std::deque<std::pair<time_t, double>> dailyAverageReadings_;
};