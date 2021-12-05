#include "globals.h"
#include "usb.h"
#include "protocol.h"
#include "fluxmap.h"
#include "bytes.h"
#include "fmt/format.h"
#include "greaseweazle.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#if defined __WIN32__
    #include <windows.h>

    typedef HANDLE FileHandle;

    static std::string get_last_error_string()
    {
        DWORD error = GetLastError();
        if (error == 0)
            return "OK";

        LPSTR buffer = nullptr;
        size_t size = FormatMessageA(
                /* dwFlags= */ FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
                /* lpSource= */ nullptr,
                /* dwMessageId= */ error,
                /* dwLanguageId= */ MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                /* lpBuffer= */ (LPSTR) &buffer,
                /* nSize= */ 0,
                /* Arguments= */ nullptr);
    
        std::string message(buffer, size);
        LocalFree(buffer);
        return message;
    }

    static HANDLE open_serial_port(const std::string& name)
    {
        HANDLE h = CreateFileA(
            name.c_str(),
            /* dwDesiredAccess= */ GENERIC_READ|GENERIC_WRITE,
            /* dwShareMode= */ 0,
            /* lpSecurityAttribues= */ nullptr,
            /* dwCreationDisposition= */ OPEN_EXISTING,
            /* dwFlagsAndAttributes= */ FILE_ATTRIBUTE_NORMAL,
            /* hTemplateFile= */ nullptr);
        if (h == INVALID_HANDLE_VALUE)
            Error() << fmt::format("cannot open GreaseWeazle serial port '{}': {}",
                name, get_last_error_string());
        
        DCB dcb =
        {
            .DCBlength = sizeof(DCB),
            .BaudRate = CBR_9600,
            .fBinary = true,
            .ByteSize = 8,
            .Parity = NOPARITY,
            .StopBits = ONESTOPBIT
        };
        SetCommState(h, &dcb);

        COMMTIMEOUTS commtimeouts = {0};
        commtimeouts.ReadIntervalTimeout = 100;
        SetCommTimeouts(h, &commtimeouts);

        PurgeComm(h, PURGE_RXABORT|PURGE_RXCLEAR|PURGE_TXABORT|PURGE_TXCLEAR);
        return h;
    }

    static void close_serial_port(FileHandle h)
    {
        CloseHandle(h);
    }

    static ssize_t read_serial_port(FileHandle h, uint8_t* buffer, size_t len)
    {
        DWORD rlen;
        bool r = ReadFile(
                /* hFile= */ h,
                /* lpBuffer= */ buffer,
                /* nNumberOfBytesToRead= */ len,
                /* lpNumberOfBytesRead= */ &rlen,
                /* lpOverlapped= */ nullptr);
        if (!r)
            return -1;
        return rlen;
    }

    static ssize_t write_serial_port(FileHandle h, const uint8_t* buffer, size_t len)
    {
        DWORD wlen;
        bool r = WriteFile(
                /* hFile= */ h,
                /* lpBuffer= */ buffer,
                /* nNumberOfBytesToWrite= */ len,
                /* lpNumberOfBytesWritten= */ &wlen,
                /* lpOverlapped= */ nullptr);
        if (!r)
            return -1;
        return wlen;
    }

#else
    #include <termios.h>

    typedef int FileHandle;
    static FileHandle open_serial_port(const std::string& name)
    {
        #ifdef __APPLE__
            if (name.find("/dev/tty.") != std::string::npos)
                std::cerr << "Warning: you probably want to be using a /dev/cu.* device\n";
        #endif

        int fd = open(name.c_str(), O_RDWR);
        if (fd == -1)
            Error() << fmt::format("cannot open GreaseWeazle serial port '{}': {}",
                name, strerror(errno));

        struct termios t;
        tcgetattr(fd, &t);
        t.c_iflag = 0;
        t.c_oflag = 0;
        t.c_cflag = CREAD;
        t.c_lflag = 0;
        t.c_cc[VMIN] = 1;
        cfsetspeed(&t, 9600);
        tcsetattr(fd, TCSANOW, &t);
        return fd;
    }

    #define close_serial_port(fd) ::close(fd)
    #define read_serial_port(fd, buf, len) ::read(fd, buf, len)
    #define write_serial_port(fd, buf, len) ::write(fd, buf, len)
#endif

static const char* gw_error(int e)
{
    switch (e)
    {
        case ACK_OKAY:           return "OK";
        case ACK_BAD_COMMAND:    return "Bad command";
        case ACK_NO_INDEX:       return "No index";
        case ACK_NO_TRK0:        return "No track 0";
        case ACK_FLUX_OVERFLOW:  return "Overflow";
        case ACK_FLUX_UNDERFLOW: return "Underflow";
        case ACK_WRPROT:         return "Write protected";
        case ACK_NO_UNIT:        return "No unit";
        case ACK_NO_BUS:         return "No bus";
        case ACK_BAD_UNIT:       return "Invalid unit";
        case ACK_BAD_PIN:        return "Invalid pin";
        case ACK_BAD_CYLINDER:   return "Invalid cylinder";
        default:                 return "Unknown error";
    }
}

class GreaseWeazleUsb : public USB
{
private:
    uint8_t _readbuffer[4096];
    int _readbuffer_ptr = 0;
    int _readbuffer_fill = 0;

    void read_bytes(uint8_t* buffer, int len)
    {
        while (len > 0)
        {
            if (_readbuffer_ptr < _readbuffer_fill)
            {
                int buffered = std::min(len, _readbuffer_fill - _readbuffer_ptr);
                memcpy(buffer, _readbuffer + _readbuffer_ptr, buffered);
                _readbuffer_ptr += buffered;
                buffer += buffered;
                len -= buffered;
            }

            if (len == 0)
                break;

            ssize_t actual = read_serial_port(_fd, _readbuffer, sizeof(_readbuffer));
            if (actual < 0)
                Error() << "failed to receive command reply: " << strerror(actual);

            _readbuffer_fill = actual;
            _readbuffer_ptr = 0;
        }
    }

    void read_bytes(Bytes& bytes)
    {
        read_bytes(bytes.begin(), bytes.size());
    }

    Bytes read_bytes(unsigned len)
    {
        Bytes b(len);
        read_bytes(b);
        return b;
    }

    uint8_t read_byte()
    {
        uint8_t b;
        read_bytes(&b, 1);
        return b;
    }

    uint32_t read_28()
    {
        return ((read_byte() & 0xfe) >> 1)
            | ((read_byte() & 0xfe) << 6)
            | ((read_byte() & 0xfe) << 13)
            | ((read_byte() & 0xfe) << 20);
    }

    void write_bytes(const uint8_t* buffer, size_t len)
    {
        while (len > 0)
        {
            ssize_t actual = write_serial_port(_fd, buffer, len);
            if (actual < 0)
                Error() << "failed to send command: " << strerror(errno);

            buffer += actual;
            len -= actual;
        }
    }

    void write_bytes(const Bytes& bytes)
    {
        write_bytes(bytes.cbegin(), bytes.size());
    }

    void do_command(const Bytes& command)
    {
        write_bytes(command);

        uint8_t buffer[2];
        read_bytes(buffer, sizeof(buffer));

        if (buffer[0] != command[0])
            Error() << fmt::format("command returned garbage (0x{:x} != 0x{:x} with status 0x{:x})",
                buffer[0], command[0], buffer[1]);
        if (buffer[1])
            Error() << fmt::format("GreaseWeazle error: {}", gw_error(buffer[1]));
    }

public:
    GreaseWeazleUsb(const std::string& port)
    {
        _fd = open_serial_port(port);

        int version = getVersion();
        if (version == 22)
            _version = V22;
        else if (version >= 24)
            _version = V24;
        else
        {
            Error() << "only GreaseWeazle firmware versions 22 and 24 or above are currently "
                    << "supported, but you have version " << version << ". Please file a bug.";
        }

        /* Configure the hardware. */

        do_command({ CMD_SET_BUS_TYPE, 3, BUS_IBMPC });
    }

    ~GreaseWeazleUsb()
    {
        close_serial_port(_fd);
    }

    int getVersion()
    {
        do_command({ CMD_GET_INFO, 3, GETINFO_FIRMWARE });

        Bytes response = read_bytes(32);
        ByteReader br(response);

        br.seek(4);
        nanoseconds_t freq = br.read_le32();
        _clock = 1000000000 / freq;

        br.seek(0);
        return br.read_be16();
    }

    void recalibrate()
    {
        seek(0);
    }
    
    void seek(int track)
    {
        do_command({ CMD_SEEK, 3, (uint8_t)track });
    }
    
    nanoseconds_t getRotationalPeriod(int hardSectorCount)
    {
        if (hardSectorCount != 0)
            Error() << "hard sectors are currently unsupported on the GreaseWeazel";

        /* The GreaseWeazle doesn't have a command to fetch the period directly,
         * so we have to do a flux read. */

        switch (_version)
        {
            case V22:
                do_command({ CMD_READ_FLUX, 2 });
                break;

            case V24:
            {
                Bytes cmd(8);
                cmd.writer()
                    .write_8(CMD_READ_FLUX)
                    .write_8(cmd.size())
                    .write_le32(0) //ticks default value (guessed)
                    .write_le32(2);//guessed
                do_command(cmd);
            }
        }

        uint32_t ticks_gw = 0;
        uint32_t firstindex = ~0;
        uint32_t secondindex = ~0;
        for (;;)
        {
            uint8_t b = read_byte();
            if (!b)
                break;

            if (b == 255)
            {
                switch (read_byte())
                {
                    case FLUXOP_INDEX:
                    {
                        uint32_t index = read_28() + ticks_gw;
                        if (firstindex == ~0)
                            firstindex = index;
                        else if (secondindex == ~0)
                            secondindex = index;
                        break;
                    }

                    case FLUXOP_SPACE:
                        read_bytes(4);
                        break;

                    default:
                        Error() << "bad opcode in GreaseWeazle stream";
                }
            }
            else
            {
                if (b < 250)
                    ticks_gw += b;
                else
                {
                    int delta = 250 + (b-250)*255 + read_byte() - 1;
                    ticks_gw += delta;
                }
            }
        }

        if (secondindex == ~0)
            Error() << "unable to determine disk rotational period (is a disk in the drive?)";
        do_command({ CMD_GET_FLUX_STATUS, 2 });

        _revolutions = (nanoseconds_t)(secondindex - firstindex) * _clock;
        return _revolutions;
    }
    
    void testBulkWrite()
    {
        const int LEN = 10*1024*1024;
        Bytes cmd(6);
        ByteWriter bw(cmd);
        bw.write_8(CMD_SINK_BYTES);
        bw.write_8(cmd.size());
        bw.write_le32(LEN);
        do_command(cmd);

        Bytes junk(LEN);
		double start_time = getCurrentTime();
        write_bytes(LEN);
        read_bytes(1);
		double elapsed_time = getCurrentTime() - start_time;

		std::cout << "Transferred "
				  << LEN
				  << " bytes from PC -> GreaseWeazle in "
				  << int(elapsed_time * 1000.0)
				  << " ms ("
				  << int((LEN / 1024.0) / elapsed_time)
				  << " kB/s)"
				  << std::endl;
    }
    
    void testBulkRead()
    {
        const int LEN = 10*1024*1024;
        Bytes cmd(6);
        ByteWriter bw(cmd);
        bw.write_8(CMD_SOURCE_BYTES);
        bw.write_8(cmd.size());
        bw.write_le32(LEN);
        do_command(cmd);

		double start_time = getCurrentTime();
        read_bytes(LEN);
		double elapsed_time = getCurrentTime() - start_time;

		std::cout << "Transferred "
				  << LEN
				  << " bytes from GreaseWeazle -> PC in "
				  << int(elapsed_time * 1000.0)
				  << " ms ("
				  << int((LEN / 1024.0) / elapsed_time)
				  << " kB/s)"
				  << std::endl;
    }

    Bytes read(int side, bool synced, nanoseconds_t readTime, nanoseconds_t hardSectorThreshold)
    {
        if (hardSectorThreshold != 0)
            Error() << "hard sectors are currently unsupported on the GreaseWeazel";

        int revolutions = (readTime+_revolutions-1) / _revolutions;

        do_command({ CMD_HEAD, 3, (uint8_t)side });

        switch (_version)
        {
            case V22:
            {
                Bytes cmd(4);
                cmd.writer()
                    .write_8(CMD_READ_FLUX)
                    .write_8(cmd.size())
                    .write_le32(revolutions + (synced ? 1 : 0));
                do_command(cmd);
                break;
            }

            case V24:
            {
                Bytes cmd(8);
                cmd.writer()
                    .write_8(CMD_READ_FLUX)
                    .write_8(cmd.size())
                    .write_le32(0) //ticks default value (guessed)
                    .write_le32(revolutions + (synced ? 1 : 0));
                do_command(cmd);
            }
        }

		Bytes buffer;
        ByteWriter bw(buffer);
        for (;;)
        {
            uint8_t b = read_byte();
            if (!b)
                break;
            bw.write_8(b);
        }

        do_command({ CMD_GET_FLUX_STATUS, 2 });

        Bytes fldata = greaseWeazleToFluxEngine(buffer, _clock);
        if (synced)
            fldata = stripPartialRotation(fldata);
        return fldata;
    }

    void write(int side, const Bytes& fldata, nanoseconds_t hardSectorThreshold)
    {
        if (hardSectorThreshold != 0)
            Error() << "hard sectors are currently unsupported on the GreaseWeazel";

        do_command({ CMD_HEAD, 3, (uint8_t)side });
        switch (_version)
        {
            case V22:
                do_command({ CMD_WRITE_FLUX, 3, 1 });
                break;

            case V24:
                do_command({ CMD_WRITE_FLUX, 4, 1, 1 });
                break;
        }
        write_bytes(fluxEngineToGreaseWeazle(fldata, _clock));
        read_byte(); /* synchronise */

        do_command({ CMD_GET_FLUX_STATUS, 2 });
    }

    void erase(int side, nanoseconds_t hardSectorThreshold)
    {
        if (hardSectorThreshold != 0)
            Error() << "hard sectors are currently unsupported on the GreaseWeazel";

        do_command({ CMD_HEAD, 3, (uint8_t)side });

        Bytes cmd(6);
        ByteWriter bw(cmd);
        bw.write_8(CMD_ERASE_FLUX);
        bw.write_8(cmd.size());
        bw.write_le32(200e6 / _clock);
        do_command(cmd);
        read_byte(); /* synchronise */

        do_command({ CMD_GET_FLUX_STATUS, 2 });
    }
    
    void setDrive(int drive, bool high_density, int index_mode)
    {
        do_command({ CMD_SELECT, 3, (uint8_t)drive });
        do_command({ CMD_MOTOR, 4, (uint8_t)drive, 1 });
        do_command({ CMD_SET_PIN, 4, 2, (uint8_t)(high_density ? 1 : 0) });
    }

    void measureVoltages(struct voltages_frame* voltages)
    { Error() << "unsupported operation on the GreaseWeazle"; }

private:
    enum
    {
	V22,
	V24
    };
    
    FileHandle _fd;
    int _version;
    nanoseconds_t _clock;
    nanoseconds_t _revolutions;
};

USB* createGreaseWeazleUsb(const std::string& port)
{
    return new GreaseWeazleUsb(port);
}

// vim: sw=4 ts=4 et

