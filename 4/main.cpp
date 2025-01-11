#include "logger.h"
#include "serial_port.h"
#include "temperature_sensor.h"
#include <chrono>
#include <iostream>
#include <thread>

int main(int argc, char *argv[]) {
    const std::string portName = "COM3";

    SerialPort serialPort(portName);

    int scale = 1;
    if (argc > 1) {
        char* endptr;
        long parsedScale = strtol(argv[1], &endptr, 10);

        if (*endptr == '\0' && parsedScale > 0) {
            scale = static_cast<int>(parsedScale);
        } else {
            std::cerr << "Invalid scale argument. Using default scale: " << scale << std::endl;
        }
    }

    TemperatureSensor sensor;

    Logger logger("all_readings.log", "hourly_average.log", "daily_average.log", scale);

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
    return 0;
}