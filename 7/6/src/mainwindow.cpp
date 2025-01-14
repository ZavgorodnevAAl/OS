#include "../include/mainwindow.h"
#include <QDateTime>
#include <QDebug>
#include <QMessageBox>

MainWindow::MainWindow(Logger &logger, QWidget *parent)
    : QMainWindow(parent), logger_(logger), serialPort_("COM3"), sensor_() {
    setWindowTitle("Temperature Monitor");

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    currentTempLabel_ = new QLabel("Current Temperature: N/A", this);
    mainLayout->addWidget(currentTempLabel_);

    plotAll_ = new Plot(this);
    plotAll_->setTitle("All Readings");
    mainLayout->addWidget(plotAll_);

    plotHourly_ = new Plot(this);
    plotHourly_->setTitle("Hourly Average");
    mainLayout->addWidget(plotHourly_);

    plotDaily_ = new Plot(this);
    plotDaily_->setTitle("Daily Average");
    mainLayout->addWidget(plotDaily_);

    setCentralWidget(centralWidget);

    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &MainWindow::updateDisplay);
    timer_->start(100);

    updateTimer_ = new QTimer(this);
    connect(updateTimer_, &QTimer::timeout, this, &MainWindow::updateReadings);
    updateTimer_->start(5000);
}

MainWindow::~MainWindow() {
    if (timer_) {
        timer_->stop();
        delete timer_;
        timer_ = nullptr;
    }
    if (updateTimer_) {
        updateTimer_->stop();
        delete updateTimer_;
        updateTimer_ = nullptr;
    }
}

void MainWindow::updateDisplay() {
    std::string temperature;
#ifdef USE_SIMULATION
    temperature = sensor_.getTemperature();
#else
    if (!serialPort_.isOpen()) {
        if (!serialPort_.openPort()) {
            qDebug() << "Failed to open port, retrying in 5 seconds...";
            return;
        } else {
            qDebug() << "Port successfully opened";
        }
    }

    temperature = serialPort_.readData();

    if (temperature.empty()) {
        return;
    }
#endif
    currentTempLabel_->setText(QString("Current Temperature: %1 °C").arg(QString::fromStdString(temperature)));
    logger_.logTemperature(temperature);
}

void MainWindow::updateReadings() {

    logger_.updateLogs();

    std::vector<std::pair<time_t, double>> allReadings = logger_.getAllReadings();
    std::vector<std::pair<time_t, double>> hourlyReadings = logger_.getHourlyAverageReadings();
    std::vector<std::pair<time_t, double>> dailyReadings = logger_.getDailyAverageReadings();

    QVector<QPair<qint64, double>> allData;
    for (const auto &reading : allReadings) {
        QDateTime dateTime = QDateTime::fromSecsSinceEpoch(reading.first);
        allData.append(QPair<qint64, double>(dateTime.toMSecsSinceEpoch(), reading.second));
    }

    QVector<QPair<qint64, double>> hourlyData;
    for (const auto &reading : hourlyReadings) {
        QDateTime dateTime = QDateTime::fromSecsSinceEpoch(reading.first);
       hourlyData.append(QPair<qint64, double>(dateTime.toMSecsSinceEpoch(), reading.second));
    }

    QVector<QPair<qint64, double>> dailyData;
    for (const auto &reading : dailyReadings) {
        QDateTime dateTime = QDateTime::fromSecsSinceEpoch(reading.first);
        dailyData.append(QPair<qint64, double>(dateTime.toMSecsSinceEpoch(), reading.second));
    }

    plotAll_->updatePlot(allData);
    plotHourly_->updatePlot(hourlyData);
    plotDaily_->updatePlot(dailyData);
}