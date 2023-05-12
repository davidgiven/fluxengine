#ifndef SERIAL_H
#define SERIAL_H

class SerialPort
{
public:
    static std::unique_ptr<SerialPort> openSerialPort(const std::string& path);

public:
    virtual ~SerialPort();
    virtual ssize_t readImpl(uint8_t* buffer, size_t len) = 0;
    virtual ssize_t write(const uint8_t* buffer, size_t len) = 0;

    void read(uint8_t* buffer, size_t len);
    void read(Bytes& bytes);
    Bytes readBytes(size_t count);
    uint8_t readByte();
    void write(const Bytes& bytes);

private:
    uint8_t _readbuffer[4096];
    size_t _readbuffer_ptr = 0;
    size_t _readbuffer_fill = 0;
};

#endif
