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
#include "lib/core/logger.h"
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <filesystem>

class Fl2Sink : public FluxSink
{
public:
    Fl2Sink(const std::string& filename): _filename(filename)
    {
        std::ofstream of(filename);
        if (!of.is_open())
            error("cannot open output file");
        of.close();
        std::filesystem::remove(filename);
    }

    ~Fl2Sink()
    {
        log("FL2: writing {}", _filename);

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

    void addFlux(int track, int head, const Fluxmap& fluxmap) override
    {
        auto& vector = _data[std::make_pair(track, head)];
        vector.push_back(fluxmap.rawBytes());
    }

private:
    std::string _filename;
    std::map<std::pair<unsigned, unsigned>, std::vector<Bytes>> _data;
};

class Fl2FluxSinkFactory : public FluxSinkFactory
{
public:
    Fl2FluxSinkFactory(const std::string& filename): _filename(filename) {}

    std::unique_ptr<FluxSink> create() override
    {
        return std::make_unique<Fl2Sink>(_filename);
    }

    std::optional<std::filesystem::path> getPath() const override
    {
        return std::make_optional(_filename);
    }

public:
    operator std::string() const override
    {
        return fmt::format("fl2({})", _filename);
    }

private:
    const std::string _filename;
};

std::unique_ptr<FluxSinkFactory> FluxSinkFactory::createFl2FluxSinkFactory(
    const Fl2FluxSinkProto& config)
{
    return std::unique_ptr<FluxSinkFactory>(
        new Fl2FluxSinkFactory(config.filename()));
}

std::unique_ptr<FluxSinkFactory> FluxSinkFactory::createFl2FluxSinkFactory(
    const std::string& filename)
{
    return std::unique_ptr<FluxSinkFactory>(new Fl2FluxSinkFactory(filename));
}
