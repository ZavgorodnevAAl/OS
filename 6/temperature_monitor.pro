SOURCES += \
    src/main.cpp \
    src/logger.cpp \
    src/serial_port.cpp \
    src/temperature_sensor.cpp \
    src/mainwindow.cpp \
    src/plot.cpp

HEADERS += \
  include/logger.h \
  include/serial_port.h \
  include/temperature_sensor.h \
  include/mainwindow.h \
  include/plot.h

QMAKE_CXXFLAGS += -DUSE_SIMULATION

unix{
LIBS += -lqwt-qt5 -lsqlite3
INCLUDEPATH += usr/include/qwt/
DEPENDPATH += usr/include/qwt/
CONFIG += qwt
CONFIG += svg
}