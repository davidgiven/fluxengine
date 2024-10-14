#include "lib/core/globals.h"
#include "lib/config/flags.h"
#include "lib/data/fluxmap.h"
#include "lib/core/bytes.h"
#include "protocol.h"
#include "lib/fluxsink/fluxsink.h"
#include "lib/data/fluxmapreader.h"
#include "lib/fluxsink/fluxsink.pb.h"
#include "lib/config/proto.h"
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>

class VcdFluxSink : public FluxSink
{
public:
    VcdFluxSink(const VcdFluxSinkProto& config): _config(config) {}

public:
    void writeFlux(int track, int head, const Fluxmap& fluxmap) override
    {
        mkdir(_config.directory().c_str(), 0744);
        std::ofstream of(
            fmt::format(
                "{}/c{:02d}.h{:01d}.vcd", _config.directory(), track, head),
            std::ios::out | std::ios::binary);
        if (!of.is_open())
            error("cannot open output file");

        of << "$timescale 1ns $end\n"
           << "$var wire 1 i index $end\n"
           << "$var wire 1 p pulse $end\n"
           << "$upscope $end\n"
           << "$enddefinitions $end\n"
           << "$dumpvars 0i 0p $end\n";

        FluxmapReader fmr(fluxmap);
        unsigned timestamp = 0;
        unsigned lasttimestamp = 0;
        while (!fmr.eof())
        {
            unsigned ticks;
            int event;
            fmr.getNextEvent(event, ticks);
            if (fmr.eof())
                break;

            unsigned newtimestamp = timestamp + ticks;
            if (newtimestamp != lasttimestamp)
            {
                of << fmt::format("\n#{} 0i 0p\n",
                    (uint64_t)((lasttimestamp + 1) * NS_PER_TICK));
                timestamp = newtimestamp;
                of << fmt::format("#{} ", (uint64_t)(timestamp * NS_PER_TICK));
            }

            if (event & F_BIT_PULSE)
                of << "1p ";
            if (event & F_BIT_INDEX)
                of << "1i ";

            lasttimestamp = timestamp;
        }
        of << "\n";
    }

    operator std::string() const override
    {
        return fmt::format("vcd({})", _config.directory());
    }

private:
    const VcdFluxSinkProto& _config;
};

std::unique_ptr<FluxSink> FluxSink::createVcdFluxSink(
    const VcdFluxSinkProto& config)
{
    return std::unique_ptr<FluxSink>(new VcdFluxSink(config));
}
