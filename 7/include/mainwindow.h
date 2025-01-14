#include <QMainWindow>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QCloseEvent>
#include "logger.h"
#include "plot.h"
#include "serial_port.h"
#include "temperature_sensor.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(Logger &logger, QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void updateDisplay();
    void updateReadings();


protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    Logger &logger_;
    QLabel *currentTempLabel_;
    QTimer *timer_;
    QTimer *updateTimer_;
    Plot *plotAll_;
    Plot *plotHourly_;
    Plot *plotDaily_;
    SerialPort serialPort_;
    TemperatureSensor sensor_;
};