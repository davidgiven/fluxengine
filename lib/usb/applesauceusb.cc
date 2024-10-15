#include "lib/core/globals.h"
#include "protocol.h"
#include "lib/data/fluxmap.h"
#include "lib/core/bytes.h"
#include "lib/usb/usb.pb.h"
#include "lib/core/utils.h"
#include "serial.h"
#include "usb.h"
#include "lib/data/fluxmapreader.h"
#include <unistd.h>

class ApplesauceUsb;
static ApplesauceUsb* instance;

static uint32_t ss_rand_next(uint32_t x)
{
    return (x & 1) ? (x >> 1) ^ 0x80000062 : x >> 1;
}

static Bytes applesauceReadDataToFluxEngine(const Bytes& asdata,
    nanoseconds_t clock,
    const std::vector<unsigned>& indexMarks)
{
    ByteReader br(asdata);
    Fluxmap fluxmap;
    auto indexIt = indexMarks.begin();
    fluxmap.appendIndex();

    unsigned totalTicks = 0;
    while (!br.eof())
    {
        uint8_t b = br.read_8();
        fluxmap.appendInterval(b * clock / NS_PER_TICK);
        if (b != 255)
            fluxmap.appendPulse();

        totalTicks += b;
        if ((indexIt != indexMarks.end()) && (totalTicks > *indexIt))
        {
            fluxmap.appendIndex();
            indexIt++;
        }
    }

    return fluxmap.rawBytes();
}

static Bytes fluxEngineToApplesauceWriteData(const Bytes& fldata)
{
    Fluxmap fluxmap(fldata);
    FluxmapReader fmr(fluxmap);
    Bytes asdata;
    ByteWriter bw(asdata);

    while (!fmr.eof())
    {
        unsigned ticks;
        if (!fmr.findEvent(F_BIT_PULSE, ticks))
            break;

        uint32_t applesauceTicks = (double)ticks * NS_PER_TICK;
        while (applesauceTicks >= 0xffff)
        {
            bw.write_le16(0xffff);
            applesauceTicks -= 0xffff;
        }
        if (applesauceTicks == 0)
            error("bad data!");
        bw.write_le16(applesauceTicks);
    }

    bw.write_le16(0);
    return asdata;
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

        doCommand("client:v2");

        atexit(
            []()
            {
                delete instance;
            });
    }

    ~ApplesauceUsb()
    {
        sendrecv("disconnect");
    }

private:
    std::string sendrecv(const std::string& command)
    {
        if (_config.verbose())
            fmt::print(fmt::format("> {}\n", command));
        _serial->writeLine(command);
        auto r = _serial->readLine();
        if (_config.verbose())
            fmt::print(fmt::format("< {}\n", r));
        return r;
    }

    void checkCommandResult(const std::string& result)
    {
        if (result != ".")
            throw ApplesauceException(
                fmt::format("low-level Applesauce error: '{}'", result));
    }

    void doCommand(const std::string& command)
    {
        checkCommandResult(sendrecv(command));
    }

    std::string doCommandX(const std::string& command)
    {
        doCommand(command);
        std::string r = _serial->readLine();
        if (_config.verbose())
            fmt::print(fmt::format("<< {}\n", r));
        return r;
    }

    void connect()
    {
        if (!_connected)
        {
            try
            {
                doCommand("connect");
                doCommand("drive:enable");
                doCommand("motor:on");
                doCommand("head:zero");
                _connected = true;
            }
            catch (const ApplesauceException& e)
            {
                error("Applesauce could not connect to a drive");
            }
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
            double period_us = std::stod(doCommandX("sync:?speed"));
            _serial->writeByte('X');
            std::string r = _serial->readLine();
            if (_config.verbose())
                fmt::print("<< {}\n", r);
            return period_us * 1e3;
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

        doCommand(fmt::format("data:>{}", max));

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

        doCommand(fmt::format("data:<{}", max));

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
        bool shortRead = readTime < 400e6;
        warning(
            "applesauce: timed reads not supported; using read of {} "
            "revolutions",
            shortRead ? "1.25" : "2.25");

        connect();
        doCommand(fmt::format("head:side{}", side));
        doCommand("sync:on");
        doCommand("data:clear");
        std::string r = doCommandX(shortRead ? "disk:read" : "disk:readx");
        auto rsplit = split(r, '|');
        if (rsplit.size() < 2)
            error("unrecognised Applesauce response to disk:read: '{}'", r);

        int bufferSize = std::stoi(rsplit[0]);
        nanoseconds_t tickSize = std::stod(rsplit[1]) / 1e3;

        std::vector<unsigned> indexMarks;
        for (auto i = rsplit.begin() + 2; i != rsplit.end(); i++)
            indexMarks.push_back(std::stoi(*i));

        doCommand(fmt::format("data:<{}", bufferSize));

        Bytes rawData = _serial->readBytes(bufferSize);
        Bytes b = applesauceReadDataToFluxEngine(rawData, tickSize, indexMarks);
        return b;
    }

private:
    void checkWritable()
    {
        if (sendrecv("disk:?write") == "-")
            error("cannot write --- disk is write protected");
        if (sendrecv("?safe") == "+")
            error("cannot write --- Applesauce 'safe' switch is on");
        if (sendrecv("?vers") < "0300")
            error("cannot write --- need Applesauce firmware 2.0 or above");
    }

public:
    void write(int side,
        const Bytes& fldata,
        nanoseconds_t hardSectorThreshold) override
    {
        if (hardSectorThreshold != 0)
            error("hard sectors are currently unsupported on the Applesauce");
        checkWritable();

        connect();
        doCommand(fmt::format("head:side{}", side));
        doCommand("sync:on");
        doCommand("disk:wipe");
        doCommand("data:clear");
        doCommand("disk:wclear");

        Bytes asdata = fluxEngineToApplesauceWriteData(fldata);
        doCommand(fmt::format("data:>{}", asdata.size()));
        _serial->write(asdata);
        checkCommandResult(_serial->readLine());
        doCommand("disk:wcmd0,0");
        doCommand("disk:write");
    }

    void erase(int side, nanoseconds_t hardSectorThreshold) override
    {
        if (hardSectorThreshold != 0)
            error("hard sectors are currently unsupported on the Applesauce");
        checkWritable();

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
    instance = new ApplesauceUsb(port, config);
    return instance;
}

// vim: sw=4 ts=4 et
