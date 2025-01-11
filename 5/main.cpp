#include "httplib/httplib.h"
#include "logger.h"
#include "serial_port.h"
#include "temperature_sensor.h"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

std::string formatTime(time_t time) {
    std::tm t;
    localtime_r(&time, &t);
    std::stringstream ss;
    ss << std::put_time(&t, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string createJsonArray(const std::vector<std::pair<time_t, double>> &data) {
    std::string json = "[";
    for (size_t i = 0; i < data.size(); ++i) {
        json += "{\"time\":\"" + formatTime(data[i].first) + "\",\"value\":" + std::to_string(data[i].second) + "}";
        if (i < data.size() - 1) {
            json += ",";
        }
    }
    json += "]";
    return json;
}

int main(int argc, char *argv[]) {
    const std::string portName = "COM3";
    SerialPort serialPort(portName);
    int scale = 1;
    if (argc > 1) {
        char *endptr;
        long parsedScale = strtol(argv[1], &endptr, 10);

        if (*endptr == '\0' && parsedScale > 0) {
            scale = static_cast<int>(parsedScale);
        } else {
            std::cerr << "Invalid scale argument. Using default scale: " << scale << std::endl;
        }
    }
    const std::string dbName = "temperature_data.db";
    TemperatureSensor sensor;
    Logger logger(dbName, scale);

    httplib::Server svr;

    svr.Get("/current", [&](const httplib::Request &, httplib::Response &res) {
        std::string temperature;
#ifdef USE_SIMULATION
        temperature = sensor.getTemperature();
#else
        if (!serialPort.isOpen()) {
            if (!serialPort.openPort()) {
                std::cerr << "Failed to open port, retrying in 5 seconds..." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(5));
                res.set_content("Failed to get temperature", "text/plain");
                return;
            } else {
                std::cout << "Port successfully opened" << std::endl;
            }
        }

        temperature = serialPort.readData();

        if (temperature.empty()) {
            res.set_content("Failed to get temperature", "text/plain");
            return;
        }
#endif
        res.set_content(temperature, "text/plain");
    });

    svr.Get("/all_readings", [&](const httplib::Request &, httplib::Response &res) {
        std::vector<std::pair<time_t, double>> readings = logger.getAllReadings();
        std::string jsonResponse = createJsonArray(readings);
        res.set_content(jsonResponse, "application/json");
    });

    svr.Get("/hourly_average", [&](const httplib::Request &, httplib::Response &res) {
        std::vector<std::pair<time_t, double>> readings = logger.getHourlyAverageReadings();
        std::string jsonResponse = createJsonArray(readings);
        res.set_content(jsonResponse, "application/json");
    });

    svr.Get("/daily_average", [&](const httplib::Request &, httplib::Response &res) {
        std::vector<std::pair<time_t, double>> readings = logger.getDailyAverageReadings();
        std::string jsonResponse = createJsonArray(readings);
        res.set_content(jsonResponse, "application/json");
    });

    std::cout << "Server is running on port 8080..." << std::endl;

    std::thread server_thread([&svr]() {
        svr.listen("0.0.0.0", 8080);
    });

    while (true) {
        std::string temperature;

#ifdef USE_SIMULATION
        temperature = sensor.getTemperature();
#else
        if (!serialPort.isOpen()) {
            if (!serialPort.openPort()) {
                std::cerr << "Failed to open port, retrying in 5 seconds..." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(5));
                continue;
            } else {
                std::cout << "Port successfully opened" << std::endl;
            }
        }

        temperature = serialPort.readData();

        if (temperature.empty()) {
            continue;
        }
#endif

        logger.logTemperature(temperature);
        logger.updateLogs();

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    server_thread.join();
    return 0;
}