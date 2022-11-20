#include "globals.h"
#include "flags.h"
#include "fluxsink/fluxsink.h"
#include "lib/config.pb.h"
#include "proto.h"
#include "utils.h"
#include "fmt/format.h"
#include <regex>

std::unique_ptr<FluxSink> FluxSink::create(const FluxSinkProto& config)
{
    switch (config.type())
    {
        case FluxSinkProto::DRIVE:
            return createHardwareFluxSink(config.drive());

        case FluxSinkProto::A2R:
            return createA2RFluxSink(config.a2r());

        case FluxSinkProto::AU:
            return createAuFluxSink(config.au());

        case FluxSinkProto::VCD:
            return createVcdFluxSink(config.vcd());

        case FluxSinkProto::SCP:
            return createScpFluxSink(config.scp());

        case FluxSinkProto::FLUX:
            return createFl2FluxSink(config.fl2());

        default:
            Error() << "bad output disk config";
            return std::unique_ptr<FluxSink>();
    }
}

void FluxSink::updateConfigForFilename(
    FluxSinkProto* proto, const std::string& filename)
{
    static const std::vector<std::pair<std::regex,
        std::function<void(const std::string&, FluxSinkProto*)>>>
        formats = {
            {std::regex("^(.*\\.a2r)$"),
             [](auto& s, auto* proto)
                {
                    proto->set_type(FluxSinkProto::A2R);
                    proto->mutable_a2r()->set_filename(s);
                }},
            {std::regex("^(.*\\.flux)$"),
             [](auto& s, auto* proto)
                {
                    proto->set_type(FluxSinkProto::FLUX);
                    proto->mutable_fl2()->set_filename(s);
                }},
            {std::regex("^(.*\\.scp)$"),
             [](auto& s, auto* proto)
                {
                    proto->set_type(FluxSinkProto::SCP);
                    proto->mutable_scp()->set_filename(s);
                }},
            {std::regex("^vcd:(.*)$"),
             [](auto& s, auto* proto)
                {
                    proto->set_type(FluxSinkProto::VCD);
                    proto->mutable_vcd()->set_directory(s);
                }},
            {std::regex("^au:(.*)$"),
             [](auto& s, auto* proto)
                {
                    proto->set_type(FluxSinkProto::AU);
                    proto->mutable_au()->set_directory(s);
                }},
            {std::regex("^drive:(.*)"),
             [](auto& s, auto* proto)
                {
                    proto->set_type(FluxSinkProto::DRIVE);
                    config.mutable_drive()->set_drive(std::stoi(s));
                }},
    };

    for (const auto& it : formats)
    {
        std::smatch match;
        if (std::regex_match(filename, match, it.first))
        {
            it.second(match[1], proto);
            return;
        }
    }

    Error() << fmt::format("unrecognised flux filename '{}'", filename);
}
