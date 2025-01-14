#include "logger.h"
#include "plot.h"
#include "serial_port.h"
#include "temperature_sensor.h"
#include <QLabel>
#include <QMainWindow>
#include <QTimer>
#include <QVBoxLayout>
#include <QVector>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(Logger &logger, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateDisplay();
    void updateReadings();

private:
    Logger &logger_;
    QLabel *currentTempLabel_;
    Plot *plotAll_;
    Plot *plotHourly_;
    Plot *plotDaily_;
    QTimer *timer_;
    QTimer *updateTimer_;
    SerialPort serialPort_;
    TemperatureSensor sensor_;
};
