#include "globals.h"
#include "usb.h"
#include "protocol.h"
#include "fluxmap.h"
#include "bytes.h"
#include "serial.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#if defined __WIN32__
#include <windows.h>

class SerialPortImpl : public SerialPort
{
public:
    SerialPortImpl(const std::string& name)
    {
        std::string dos_name = "\\\\.\\" + name;
        _handle = CreateFileA(dos_name.c_str(),
            /* dwDesiredAccess= */ GENERIC_READ | GENERIC_WRITE,
            /* dwShareMode= */ 0,
            /* lpSecurityAttribues= */ nullptr,
            /* dwCreationDisposition= */ OPEN_EXISTING,
            /* dwFlagsAndAttributes= */ FILE_ATTRIBUTE_NORMAL,
            /* hTemplateFile= */ nullptr);
        if (_handle == INVALID_HANDLE_VALUE)
            error("cannot open serial port '{}': {}",
                name,
                get_last_error_string());

        DCB dcb = {.DCBlength = sizeof(DCB),
            .BaudRate = CBR_9600,
            .fBinary = true,
            .ByteSize = 8,
            .Parity = NOPARITY,
            .StopBits = ONESTOPBIT};
        SetCommState(_handle, &dcb);

        COMMTIMEOUTS commtimeouts = {0};
        commtimeouts.ReadIntervalTimeout = 100;
        SetCommTimeouts(_handle, &commtimeouts);

        if (!EscapeCommFunction(_handle, CLRDTR))
            error("Couldn't clear DTR: {}", get_last_error_string());
        Sleep(200);
        if (!EscapeCommFunction(_handle, SETDTR))
            error("Couldn't set DTR: {}", get_last_error_string());

        PurgeComm(_handle,
            PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);
    }

    ~SerialPortImpl() override
    {
        CloseHandle(_handle);
    }

public:
    ssize_t readImpl(uint8_t* buffer, size_t len) override
    {
        DWORD rlen;
        bool r = ReadFile(
            /* hFile= */ _handle,
            /* lpBuffer= */ buffer,
            /* nNumberOfBytesToRead= */ len,
            /* lpNumberOfBytesRead= */ &rlen,
            /* lpOverlapped= */ nullptr);
        if (!r)
            error("serial read I/O error: {}", get_last_error_string());
        return rlen;
    }

    ssize_t write(const uint8_t* buffer, size_t len) override
    {
        DWORD wlen;
        /* Windows gets unhappy if we try to transfer too much... */
        len = std::min(4096U, len);
        bool r = WriteFile(
            /* hFile= */ _handle,
            /* lpBuffer= */ buffer,
            /* nNumberOfBytesToWrite= */ len,
            /* lpNumberOfBytesWritten= */ &wlen,
            /* lpOverlapped= */ nullptr);
        if (!r)
            error("serial write I/O error: {}", get_last_error_string());
        return wlen;
    }

private:
    static std::string get_last_error_string()
    {
        DWORD error = GetLastError();
        if (error == 0)
            return "OK";

        LPSTR buffer = nullptr;
        size_t size = FormatMessageA(
            /* dwFlags= */ FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            /* lpSource= */ nullptr,
            /* dwMessageId= */ error,
            /* dwLanguageId= */ MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            /* lpBuffer= */ (LPSTR)&buffer,
            /* nSize= */ 0,
            /* Arguments= */ nullptr);

        std::string message(buffer, size);
        LocalFree(buffer);
        return message;
    }

private:
    HANDLE _handle;
};

#else
#include <termios.h>
#include <sys/ioctl.h>

class SerialPortImpl : public SerialPort
{
public:
    SerialPortImpl(const std::string& path)
    {
#ifdef __APPLE__
        if (path.find("/dev/tty.") != std::string::npos)
            std::cerr << "Warning: you probably want to be using a /dev/cu.* "
                         "device\n";
#endif

        _fd = open(path.c_str(), O_RDWR);
        if (_fd == -1)
            error("cannot open serial port '{}': {}", path, strerror(errno));

        struct termios t;
        tcgetattr(_fd, &t);
        t.c_iflag = 0;
        t.c_oflag = 0;
        t.c_cflag = CREAD;
        t.c_lflag = 0;
        t.c_cc[VMIN] = 1;
        cfsetspeed(&t, 9600);
        tcsetattr(_fd, TCSANOW, &t);

        /* Toggle DTR to reset the device. */

        int flag = TIOCM_DTR;
        if (ioctl(_fd, TIOCMBIC, &flag) == -1)
            error("cannot clear DTR on serial port: {}", strerror(errno));
        usleep(200000);
        if (ioctl(_fd, TIOCMBIS, &flag) == -1)
            error("cannot set DTR on serial port: {}", strerror(errno));

        /* Flush pending input from a generic greaseweazel device */
        tcsetattr(_fd, TCSAFLUSH, &t);
    }

    ~SerialPortImpl() override
    {
        close(_fd);
    }

public:
    ssize_t readImpl(uint8_t* buffer, size_t len) override
    {
        ssize_t rlen = ::read(_fd, buffer, len);
        if (rlen == 0)
            error("serial read returned no data (device removed?)");
        if (rlen == -1)
            error("serial read I/O error: {}", strerror(errno));
        return rlen;
    }

    ssize_t write(const uint8_t* buffer, size_t len) override
    {
        ssize_t wlen = ::write(_fd, buffer, len);
        if (wlen == -1)
            error("serial write I/O error: {}", strerror(errno));
        return wlen;
    }

private:
    int _fd;
};
#endif

SerialPort::~SerialPort() {}

void SerialPort::read(uint8_t* buffer, size_t len)
{
    while (len != 0)
    {
        // std::cout << "want " << len << " " << std::flush;
        size_t rlen = this->readImpl(buffer, len);
        // std::cout << "got " << rlen << "\n" << std::flush;
        buffer += rlen;
        len -= rlen;
    }
}

void SerialPort::read(Bytes& bytes)
{
    this->read(bytes.begin(), bytes.size());
}

Bytes SerialPort::readBytes(size_t len)
{
    Bytes b(len);
    this->read(b);
    return b;
}

uint8_t SerialPort::readByte()
{
    uint8_t b;
    this->read(&b, 1);
    return b;
}

void SerialPort::write(const Bytes& bytes)
{
    int ptr = 0;
    while (ptr < bytes.size())
    {
        ssize_t wlen = this->write(bytes.cbegin() + ptr, bytes.size() - ptr);
        ptr += wlen;
    }
}

std::unique_ptr<SerialPort> SerialPort::openSerialPort(const std::string& path)
{
    return std::make_unique<SerialPortImpl>(path);
}
