#include "../include/mainwindow.h"
#include <QApplication>
#include <iostream>

int main(int argc, char *argv[]) {

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

    QApplication a(argc, argv);
    const std::string dbName = "temperature_data.db";
    Logger logger(dbName, scale);
    MainWindow w(logger);
    w.show();
    return a.exec();
}