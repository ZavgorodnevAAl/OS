#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#endif

class SerialPort {
public:
    SerialPort(const std::string &portName);
    ~SerialPort();

    bool openPort();
    bool closePort();
    std::string readData();

    bool isOpen() const;

private:
    std::string portName_;

#ifdef _WIN32
    HANDLE hSerial;
#else
    int fd_;
#endif
    bool connected_;

#ifdef _WIN32
    bool setupPort();
#endif
};