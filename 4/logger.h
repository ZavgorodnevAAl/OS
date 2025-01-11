#include <ctime>
#include <deque>
#include <fstream>
#include <string>
#include <vector>

class Logger {
public:
    Logger(const std::string &allReadingsLog, const std::string &hourlyAverageLog, const std::string &dailyAverageLog, int scale);
    Logger(const std::string &allReadingsLog, const std::string &hourlyAverageLog, const std::string &dailyAverageLog);
    ~Logger();
    void logTemperature(const std::string &temperature);
    void updateLogs();

private:
    std::string allReadingsLog_;
    std::string hourlyAverageLog_;
    std::string dailyAverageLog_;
    std::deque<std::pair<time_t, double>> temperatureReadings_;
    std::deque<std::pair<time_t, double>> hourlyAverageReadings_;
    std::deque<std::pair<time_t, double>> dailyAverageReadings_;

    time_t getCurrentTime();
    void writeLog(const std::string &fileName, const std::string &message, bool append);
    void calculateHourlyAverage();
    void calculateDailyAverage();
    void cleanupLogs();

    int simulationScale_;
};
