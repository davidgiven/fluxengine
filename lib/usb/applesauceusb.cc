#include "lib/globals.h"
#include "protocol.h"
#include "lib/fluxmap.h"
#include "lib/bytes.h"
#include "lib/usb/usb.pb.h"
#include "applesauce.h"
#include "serial.h"
#include "usb.h"
#include <unistd.h>

static uint32_t ss_rand_next(uint32_t x)
{
    return (x & 1) ? (x >> 1) ^ 0x80000062 : x >> 1;
}

class ApplesauceUsb : public USB
{
public:
    ApplesauceUsb(const std::string& port, const ApplesauceProto& config):
        _serial(SerialPort::openSerialPort(port)),
        _config(config)
    {
        std::string s = sendrecv("?");
        if (s != "Applesauce")
            error(
                "Applesauce device not responding (expected 'Applesauce', got "
                "'{}')",
                s);
    }

    ~ApplesauceUsb()
    {
        sendrecv("disconnect");
    }

private:
    std::string sendrecv(const std::string& command)
    {
        _serial->writeLine(command);
        return _serial->readLine();
    }

    void connect()
    {
        if (!_connected)
        {
            std::string probe = sendrecv("connect");
            if (probe != ".")
                error("Applesauce could not find any drives");
            _connected = true;
        }
    }

public:
    int getVersion() override
    {
        // do_command({CMD_GET_INFO, 3, GETINFO_FIRMWARE});
        //
        // Bytes response = _serial->readBytes(32);
        // ByteReader br(response);
        //
        // br.seek(4);
        // nanoseconds_t freq = br.read_le32();
        // _clock = 1000000000 / freq;
        //
        // br.seek(0);
        // return br.read_be16();
        error("unsupported operation getVersion on the Greaseweazle");
    }

    void seek(int track) override
    {
        // do_command({CMD_SEEK, 3, (uint8_t)track});
        error("unsupported operation seek on the Greaseweazle");
    }

    nanoseconds_t getRotationalPeriod(int hardSectorCount) override
    {
        // if (hardSectorCount != 0)
        //     error("hard sectors are currently unsupported on the
        //     Greaseweazle");

        // /* The Greaseweazle doesn't have a command to fetch the period
        // directly,
        //  * so we have to do a flux read. */

        // switch (_version)
        // {
        //     case V22:
        //         do_command({CMD_READ_FLUX, 2});
        //         break;

        //     case V24:
        //     case V29:
        //     {
        //         Bytes cmd(8);
        //         cmd.writer()
        //             .write_8(CMD_READ_FLUX)
        //             .write_8(cmd.size())
        //             .write_le32(0)  // ticks default value (guessed)
        //             .write_le16(2); // revolutions
        //         do_command(cmd);
        //     }
        // }

        // uint32_t ticks_gw = 0;
        // uint32_t firstindex = ~0;
        // uint32_t secondindex = ~0;
        // for (;;)
        // {
        //     uint8_t b = _serial->readByte();
        //     if (!b)
        //         break;

        //     if (b == 255)
        //     {
        //         switch (_serial->readByte())
        //         {
        //             case FLUXOP_INDEX:
        //             {
        //                 uint32_t index = read_28() + ticks_gw;
        //                 if (firstindex == ~0)
        //                     firstindex = index;
        //                 else if (secondindex == ~0)
        //                     secondindex = index;
        //                 break;
        //             }

        //             case FLUXOP_SPACE:
        //                 ticks_gw += read_28();
        //                 break;

        //             default:
        //                 error("bad opcode in Greaseweazle stream");
        //         }
        //     }
        //     else
        //     {
        //         if (b < 250)
        //             ticks_gw += b;
        //         else
        //         {
        //             int delta = 250 + (b - 250) * 255 + _serial->readByte() -
        //             1; ticks_gw += delta;
        //         }
        //     }
        // }

        // if (secondindex == ~0)
        //     error(
        //         "unable to determine disk rotational period (is a disk in the
        //         " "drive?)");
        // do_command({CMD_GET_FLUX_STATUS, 2});

        // _revolutions = (nanoseconds_t)(secondindex - firstindex) * _clock;
        // return _revolutions;
        error("unsupported operation getRotationalPeriod on the Greaseweazle");
    }

    void testBulkWrite() override
    {
        int max = std::stoi(sendrecv("data:?max"));
        fmt::print("Writing data: ");

        if (sendrecv(fmt::format("data:>{}", max)) != ".")
            error("Cannot write to Applesauce");

        Bytes junk(max);
        uint32_t seed = 0;
        for (int i = 0; i < max; i++)
        {
            junk[i] = seed;
            seed = ss_rand_next(seed);
        }
        double start_time = getCurrentTime();
        _serial->write(junk);
        _serial->readLine();
        double elapsed_time = getCurrentTime() - start_time;

        std::cout << fmt::format(
            "transferred {} bytes from PC -> device in {} ms ({} kb/s)\n",
            max,
            int(elapsed_time * 1000.0),
            int((max / 1024.0) / elapsed_time));
    }

    void testBulkRead() override
    {
        int max = std::stoi(sendrecv("data:?max"));
        fmt::print("Reading data: ");

        if (sendrecv(fmt::format("data:<{}", max)) != ".")
            error("Cannot read from Applesauce");

        double start_time = getCurrentTime();
        _serial->readBytes(max);
        double elapsed_time = getCurrentTime() - start_time;

        std::cout << fmt::format(
            "transferred {} bytes from device -> PC in {} ms ({} kb/s)\n",
            max,
            int(elapsed_time * 1000.0),
            int((max / 1024.0) / elapsed_time));
    }

    Bytes read(int side,
        bool synced,
        nanoseconds_t readTime,
        nanoseconds_t hardSectorThreshold) override
    {
        // if (hardSectorThreshold != 0)
        // error("hard sectors are currently unsupported on the Greaseweazle");
        //
        // do_command({CMD_HEAD, 3, (uint8_t)side});
        //
        // switch (_version)
        // {
        // case V22:
        // {
        // int revolutions = (readTime + _revolutions - 1) / _revolutions;
        // Bytes cmd(4);
        // cmd.writer()
        // .write_8(CMD_READ_FLUX)
        // .write_8(cmd.size())
        // .write_le32(revolutions + (synced ? 1 : 0));
        // do_command(cmd);
        // break;
        // }
        //
        // case V24:
        // case V29:
        // {
        // Bytes cmd(8);
        // cmd.writer()
        // .write_8(CMD_READ_FLUX)
        // .write_8(cmd.size())
        // .write_le32(
        // (readTime + (synced ? _revolutions : 0)) / _clock)
        // .write_le16(0);
        // do_command(cmd);
        // }
        // }
        //
        // Bytes buffer;
        // ByteWriter bw(buffer);
        // for (;;)
        // {
        // uint8_t b = _serial->readByte();
        // if (!b)
        // break;
        // bw.write_8(b);
        // }
        //
        // do_command({CMD_GET_FLUX_STATUS, 2});
        //
        // Bytes fldata = greaseWeazleToFluxEngine(buffer, _clock);
        // if (synced)
        // fldata = stripPartialRotation(fldata);
        // return fldata;
        error("unsupported operation read on the Greaseweazle");
    }

    void write(int side,
        const Bytes& fldata,
        nanoseconds_t hardSectorThreshold) override
    {
        // if (hardSectorThreshold != 0)
        // error("hard sectors are currently unsupported on the Greaseweazle");
        //
        // do_command({CMD_HEAD, 3, (uint8_t)side});
        // switch (_version)
        // {
        // case V22:
        // do_command({CMD_WRITE_FLUX, 3, 1});
        // break;
        //
        // case V24:
        // case V29:
        // do_command({CMD_WRITE_FLUX, 4, 1, 1});
        // break;
        // }
        // _serial->write(fluxEngineToGreaseweazle(fldata, _clock));
        // _serial->readByte(); /* synchronise */
        //
        // do_command({CMD_GET_FLUX_STATUS, 2});
        error("unsupported operation write on the Greaseweazle");
    }

    void erase(int side, nanoseconds_t hardSectorThreshold) override
    {
        // if (hardSectorThreshold != 0)
        // error("hard sectors are currently unsupported on the Greaseweazle");
        //
        // do_command({CMD_HEAD, 3, (uint8_t)side});
        //
        // Bytes cmd(6);
        // ByteWriter bw(cmd);
        // bw.write_8(CMD_ERASE_FLUX);
        // bw.write_8(cmd.size());
        // bw.write_le32(200e6 / _clock);
        // do_command(cmd);
        // _serial->readByte(); /* synchronise */
        //
        // do_command({CMD_GET_FLUX_STATUS, 2});
        error("unsupported operation erase on the Greaseweazle");
    }

    void setDrive(int drive, bool high_density, int index_mode) override
    {
        if (drive != 0)
            error("the Applesauce only supports drive 0");

        connect();
        sendrecv("drive:enable");
        sendrecv("motor:on");
        sendrecv(fmt::format("dpc:density{}", high_density));
    }

    void measureVoltages(struct voltages_frame* voltages) override
    {
        error("unsupported operation on the Greaseweazle");
    }

private:
    std::unique_ptr<SerialPort> _serial;
    const ApplesauceProto& _config;
    bool _connected = false;
};

USB* createApplesauceUsb(const std::string& port, const ApplesauceProto& config)
{
    return new ApplesauceUsb(port, config);
}

// vim: sw=4 ts=4 et
