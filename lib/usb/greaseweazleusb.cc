#include "globals.h"
#include "usb.h"
#include "protocol.h"
#include "fluxmap.h"
#include "bytes.h"
#include "fmt/format.h"
#include "lib/usb/usb.pb.h"
#include "greaseweazle.h"
#include "serial.h"

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

static uint32_t ss_rand_next(uint32_t x)
{
    return (x&1) ? (x>>1) ^ 0x80000062 : x>>1;
}

class GreaseWeazleUsb : public USB
{
private:
    uint32_t read_28()
    {
        uint8_t buffer[4];
        _serial->read(buffer, sizeof(buffer));

        return ((buffer[0] & 0xfe) >> 1)
            | ((buffer[1] & 0xfe) << 6)
            | ((buffer[2] & 0xfe) << 13)
            | ((buffer[3] & 0xfe) << 20);
    }

    void do_command(const Bytes& command)
    {
        _serial->write(command);

        uint8_t buffer[2];
        _serial->read(buffer, sizeof(buffer));

        if (buffer[0] != command[0])
            Error() << fmt::format("command returned garbage (0x{:x} != 0x{:x} with status 0x{:x})",
                buffer[0], command[0], buffer[1]);
        if (buffer[1])
            Error() << fmt::format("GreaseWeazle error: {}", gw_error(buffer[1]));
    }

public:
    GreaseWeazleUsb(const std::string& port, const GreaseWeazleProto& config):
            _serial(SerialPort::openSerialPort(port)),
            _config(config)
    {
        int version = getVersion();
        if (version >= 29)
            _version = V29;
        else if (version >= 24)
            _version = V24;
        else if (version == 22)
            _version = V22;
        else
        {
            Error() << "only GreaseWeazle firmware versions 22 and 24 or above are currently "
                    << "supported, but you have version " << version << ". Please file a bug.";
        }

        /* Configure the hardware. */

        do_command({ CMD_SET_BUS_TYPE, 3, (uint8_t)config.bus_type() });
    }

    int getVersion()
    {
        do_command({ CMD_GET_INFO, 3, GETINFO_FIRMWARE });

        Bytes response = _serial->readBytes(32);
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
            case V29:
            {
                Bytes cmd(8);
                cmd.writer()
                    .write_8(CMD_READ_FLUX)
                    .write_8(cmd.size())
                    .write_le32(0) //ticks default value (guessed)
                    .write_le16(2);//revolutions
                do_command(cmd);
            }
        }

        uint32_t ticks_gw = 0;
        uint32_t firstindex = ~0;
        uint32_t secondindex = ~0;
        for (;;)
        {
            uint8_t b = _serial->readByte();
            if (!b)
                break;

            if (b == 255)
            {
                switch (_serial->readByte())
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
                        _serial->readBytes(4);
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
                    int delta = 250 + (b-250)*255 + _serial->readByte() - 1;
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
        std::cout << "Writing data: " << std::flush;
        const int LEN = 10*1024*1024;
        Bytes cmd;
        switch (_version)
        {
            case V22:
            case V24:
            {
                cmd.resize(6);
                ByteWriter bw(cmd);
                bw.write_8(CMD_SINK_BYTES);
                bw.write_8(cmd.size());
                bw.write_le32(LEN);
                break;
            }

            case V29:
            {
                cmd.resize(10);
                ByteWriter bw(cmd);
                bw.write_8(CMD_SINK_BYTES);
                bw.write_8(cmd.size());
                bw.write_le32(LEN);
                bw.write_le32(0); // seed
                break;
            }
        }
        do_command(cmd);

        Bytes junk(LEN);
        uint32_t seed = 0;
        for (int i=0; i<LEN; i++)
        {
            junk[i] = seed;
            seed = ss_rand_next(seed);
        }
		double start_time = getCurrentTime();
        _serial->write(junk);
        _serial->readBytes(1);
		double elapsed_time = getCurrentTime() - start_time;

        std::cout << fmt::format("transferred {} bytes from PC -> device in {} ms ({} kb/s)\n",
                LEN, int(elapsed_time * 1000.0), int((LEN / 1024.0) / elapsed_time));
    }
    
    void testBulkRead()
    {
        std::cout << "Reading data: " << std::flush;
        const int LEN = 10*1024*1024;
        Bytes cmd;
        switch (_version)
        {
            case V22:
            case V24:
            {
                cmd.resize(6);
                ByteWriter bw(cmd);
                bw.write_8(CMD_SOURCE_BYTES);
                bw.write_8(cmd.size());
                bw.write_le32(LEN);
                break;
            }

            case V29:
            {
                cmd.resize(10);
                ByteWriter bw(cmd);
                bw.write_8(CMD_SOURCE_BYTES);
                bw.write_8(cmd.size());
                bw.write_le32(LEN);
                bw.write_le32(0); // seed
                break;
            }
        }
        do_command(cmd);

		double start_time = getCurrentTime();
        _serial->readBytes(LEN);
		double elapsed_time = getCurrentTime() - start_time;

        std::cout << fmt::format("transferred {} bytes from device -> PC in {} ms ({} kb/s)\n",
                LEN, int(elapsed_time * 1000.0), int((LEN / 1024.0) / elapsed_time));
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
            case V29:
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
            uint8_t b = _serial->readByte();
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
            case V29:
                do_command({ CMD_WRITE_FLUX, 4, 1, 1 });
                break;
        }
        _serial->write(fluxEngineToGreaseWeazle(fldata, _clock));
        _serial->readByte(); /* synchronise */

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
        _serial->readByte(); /* synchronise */

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
        V24,
        V29
    };
    
    std::unique_ptr<SerialPort> _serial;
    const GreaseWeazleProto& _config;
    int _version;
    nanoseconds_t _clock;
    nanoseconds_t _revolutions;
};

USB* createGreaseWeazleUsb(const std::string& port, const GreaseWeazleProto& config)
{
    return new GreaseWeazleUsb(port, config);
}

// vim: sw=4 ts=4 et
