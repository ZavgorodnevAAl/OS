#include "temperature_sensor.h"
#include <iomanip>
#include <random>
#include <sstream>

TemperatureSensor::TemperatureSensor() : currentTemperature_(20.0) {
}

std::string TemperatureSensor::getTemperature() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> distrib(0, 0.5);

    currentTemperature_ += distrib(gen);

    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << currentTemperature_;
    return ss.str();
}