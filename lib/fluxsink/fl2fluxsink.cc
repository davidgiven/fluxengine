#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/config/flags.h"
#include "lib/data/fluxmap.h"
#include "lib/core/bytes.h"
#include "protocol.h"
#include "lib/fluxsink/fluxsink.h"
#include "lib/data/fluxmapreader.h"
#include "lib/fluxsink/fluxsink.pb.h"
#include "lib/config/proto.h"
#include "lib/external/fl2.pb.h"
#include "lib/external/fl2.h"
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <filesystem>

class Fl2FluxSink : public FluxSink
{
public:
    Fl2FluxSink(const Fl2FluxSinkProto& lconfig):
        Fl2FluxSink(lconfig.filename())
    {
    }

    Fl2FluxSink(const std::string& filename): _filename(filename)
    {
        std::ofstream of(filename);
        if (!of.is_open())
            error("cannot open output file");
        of.close();
        std::filesystem::remove(filename);
    }

    ~Fl2FluxSink()
    {
        FluxFileProto proto;
        for (const auto& e : _data)
        {
            auto track = proto.add_track();
            track->set_track(e.first.first);
            track->set_head(e.first.second);
            for (const auto& fluxBytes : e.second)
                track->add_flux(fluxBytes);
        }

        proto.set_rotational_period_ms(
            globalConfig()->drive().rotational_period_ms());
        proto.set_drive_type(globalConfig()->drive().drive_type());
        proto.set_format_type(globalConfig()->layout().format_type());
        saveFl2File(_filename, proto);
    }

public:
    void writeFlux(int track, int head, const Fluxmap& fluxmap) override
    {
        auto& vector = _data[std::make_pair(track, head)];
        vector.push_back(fluxmap.rawBytes());
    }

    operator std::string() const override
    {
        return fmt::format("fl2({})", _filename);
    }

private:
    std::string _filename;
    std::map<std::pair<unsigned, unsigned>, std::vector<Bytes>> _data;
};

std::unique_ptr<FluxSink> FluxSink::createFl2FluxSink(
    const Fl2FluxSinkProto& config)
{
    return std::unique_ptr<FluxSink>(new Fl2FluxSink(config));
}

std::unique_ptr<FluxSink> FluxSink::createFl2FluxSink(
    const std::string& filename)
{
    return std::unique_ptr<FluxSink>(new Fl2FluxSink(filename));
}
