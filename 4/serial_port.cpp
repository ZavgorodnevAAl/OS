#include "serial_port.h"
#include <chrono>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#else
#include <cstring>
#include <errno.h>
#endif

SerialPort::SerialPort(const std::string &portName) : portName_(portName), connected_(false) {
#ifdef _WIN32
    hSerial = INVALID_HANDLE_VALUE;
#else
    fd_ = -1;
#endif
}

SerialPort::~SerialPort() {
    closePort();
}

bool SerialPort::openPort() {
    if (connected_) {
        return true;
    }

#ifdef _WIN32
    hSerial = CreateFileA(portName.c_str(),
                          GENERIC_READ,
                          0,
                          NULL,
                          OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL);

    if (hSerial == INVALID_HANDLE_VALUE) {
        std::cerr << "Error opening serial port: " << GetLastError() << std::endl;
        return false;
    }

    if (!setupPort()) {
        closePort();
        return false;
    }

#else
    fd_ = open(portName_.c_str(), O_RDWR | O_NOCTTY);
    if (fd_ == -1) {

#ifdef _WIN32
        LPSTR messageBuffer = nullptr;
        DWORD errorMessageId = GetLastError();
        size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                     NULL, errorMessageId, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

        std::string message(messageBuffer, size);
        LocalFree(messageBuffer);
        std::cerr << "Error opening serial port: " << message << std::endl;
#else
        std::cerr << "Error opening serial port: " << strerror(errno) << std::endl;
#endif

        closePort();
        return false;
    }

    termios tty;
    if (tcgetattr(fd_, &tty) != 0) {
#ifdef _WIN32
        LPSTR messageBuffer = nullptr;
        DWORD errorMessageId = GetLastError();
        size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                     NULL, errorMessageId, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

        std::string message(messageBuffer, size);
        LocalFree(messageBuffer);
        std::cerr << "Error from tcgetattr: " << message << std::endl;
#else
        std::cerr << "Error from tcgetattr: " << strerror(errno) << std::endl;
#endif

        closePort();
        return false;
    }

    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= CREAD | CLOCAL;
    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO;
    tty.c_lflag &= ~ECHOE;
    tty.c_lflag &= ~ECHONL;
    tty.c_lflag &= ~ISIG;

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);

    tty.c_oflag &= ~OPOST;

    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 0;

    cfsetospeed(&tty, B9600);
    cfsetispeed(&tty, B9600);

    if (tcsetattr(fd_, TCSANOW, &tty) != 0) {

#ifdef _WIN32
        LPSTR messageBuffer = nullptr;
        DWORD errorMessageId = GetLastError();
        size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                     NULL, errorMessageId, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

        std::string message(messageBuffer, size);
        LocalFree(messageBuffer);
        std::cerr << "Error from tcsetattr: " << message << std::endl;
#else
        std::cerr << "Error from tcsetattr: " << strerror(errno) << std::endl;
#endif

        closePort();
        return false;
    }

#endif

    connected_ = true;
    return true;
}

bool SerialPort::closePort() {
    if (!connected_)
        return true;

#ifdef _WIN32
    if (hSerial != INVALID_HANDLE_VALUE) {
        CloseHandle(hSerial);
        hSerial = INVALID_HANDLE_VALUE;
    }
#else
    if (fd_ != -1) {
        close(fd_);
        fd_ = -1;
    }
#endif

    connected_ = false;
    return true;
}

std::string SerialPort::readData() {
    if (!connected_) {
        return "";
    }

    const int bufferSize = 256;
    char buffer[bufferSize];
    std::string data;

#ifdef _WIN32
    DWORD bytesRead;
    if (!ReadFile(hSerial, buffer, bufferSize - 1, &bytesRead, NULL) || bytesRead == 0) {
        return "";
    }
    buffer[bytesRead] = '\0';
    data = buffer;
#else
    int bytesRead = read(fd_, buffer, bufferSize - 1);
    if (bytesRead <= 0) {
        return "";
    }
    buffer[bytesRead] = '\0';
    data = buffer;
#endif

    return data;
}

bool SerialPort::isOpen() const {
    return connected_;
}

#ifdef _WIN32
bool SerialPort::setupPort() {
    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(hSerial, &dcbSerialParams)) {

        LPSTR messageBuffer = nullptr;
        DWORD errorMessageId = GetLastError();
        size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                     NULL, errorMessageId, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

        std::string message(messageBuffer, size);
        LocalFree(messageBuffer);
        std::cerr << "Error getting port state: " << message << std::endl;

        return false;
    }

    dcbSerialParams.BaudRate = CBR_9600;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    if (!SetCommState(hSerial, &dcbSerialParams)) {

        LPSTR messageBuffer = nullptr;
        DWORD errorMessageId = GetLastError();
        size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                     NULL, errorMessageId, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

        std::string message(messageBuffer, size);
        LocalFree(messageBuffer);
        std::cerr << "Error setting port state: " << message << std::endl;
        return false;
    }

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    if (!SetCommTimeouts(hSerial, &timeouts)) {

        LPSTR messageBuffer = nullptr;
        DWORD errorMessageId = GetLastError();
        size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                     NULL, errorMessageId, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

        std::string message(messageBuffer, size);
        LocalFree(messageBuffer);
        std::cerr << "Error setting timeouts: " << message << std::endl;
        return false;
    }

    return true;
}
#endif
