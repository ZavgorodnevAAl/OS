#include <string>

class TemperatureSensor {
public:
    TemperatureSensor();
    std::string getTemperature();

private:
    double currentTemperature_;
};