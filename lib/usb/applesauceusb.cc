#include "lib/globals.h"
#include "protocol.h"
#include "lib/fluxmap.h"
#include "lib/bytes.h"
#include "lib/usb/usb.pb.h"
#include "lib/utils.h"
#include "applesauce.h"
#include "serial.h"
#include "usb.h"
#include <unistd.h>

static uint32_t ss_rand_next(uint32_t x)
{
    return (x & 1) ? (x >> 1) ^ 0x80000062 : x >> 1;
}

class ApplesauceException : public ErrorException
{
public:
    ApplesauceException(const std::string& s): ErrorException(s) {}
};

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
        fmt::print(fmt::format("> {}\n", command));
        _serial->writeLine(command);
        auto r = _serial->readLine();
        fmt::print(fmt::format("< {}\n", r));
        return r;
    }

    bool doCommand(const std::string& command)
    {
        return sendrecv(command) == ".";
    }

    std::string doCommandX(const std::string& command)
    {
        std::string r = sendrecv(command);
        if (r != ".")
            throw ApplesauceException(
                fmt::format("low-level Applesauce error: '{}'", r));
        r = _serial->readLine();
        fmt::print(fmt::format("<< {}\n", r));
        return r;
    }

    void connect()
    {
        if (!_connected)
        {
            if (!doCommand("connect"))
                error("Applesauce could not find any drives");
            doCommand("drive:enable");
            doCommand("motor:on");
            _connected = true;
        }
    }

public:
    void seek(int track) override
    {
        if (track == 0)
            doCommand("head:zero");
        else
            doCommand(fmt::format("head:track{}", track));
    }

    nanoseconds_t getRotationalPeriod(int hardSectorCount) override
    {
        if (hardSectorCount != 0)
            error("hard sectors are currently unsupported on the Applesauce");

        connect();
        try
        {
            double rpm = std::stod(doCommandX("sync:?speed")) / 1000.0;
            sendrecv("X");
            fmt::print("< {}\n", _serial->readLine());
            return 60e9 / rpm;
        }
        catch (const ApplesauceException& e)
        {
            return 0;
        }
    }

    void testBulkWrite() override
    {
        int max = std::stoi(sendrecv("data:?max"));
        fmt::print("Writing data: ");

        if (!doCommand(fmt::format("data:>{}", max)))
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

        if (!doCommand(fmt::format("data:<{}", max)))
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
        if (hardSectorThreshold != 0)
            error("hard sectors are currently unsupported on the Applesauce");

        connect();
        doCommand(fmt::format("disk:side{}", side));
        doCommand(synced ? "sync:on" : "sync:off");
        doCommand("data:clear");
        doCommandX("disk:read");

        int bufferSize = std::stoi(sendrecv("data:?size"));

        doCommand(fmt::format("data:<{}", bufferSize));

        Bytes rawData = _serial->readBytes(bufferSize);
        return applesauceToFluxEngine(rawData);
    }

    void write(int side,
        const Bytes& fldata,
        nanoseconds_t hardSectorThreshold) override
    {
        if (hardSectorThreshold != 0)
            error("hard sectors are currently unsupported on the Applesauce");
        error("unsupported operation write on the Greaseweazle");
    }

    void erase(int side, nanoseconds_t hardSectorThreshold) override
    {
        if (hardSectorThreshold != 0)
            error("hard sectors are currently unsupported on the Applesauce");

        connect();
        doCommand(fmt::format("disk:side{}", side));
        doCommand("disk:wipe");
    }

    void setDrive(int drive, bool high_density, int index_mode) override
    {
        if (drive != 0)
            error("the Applesauce only supports drive 0");

        connect();
        doCommand(fmt::format("dpc:density{}", high_density ? '+' : '-'));
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
