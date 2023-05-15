#include "globals.h"
#include "flags.h"
#include "fluxsink/fluxsink.h"
#include "lib/config.pb.h"
#include "proto.h"
#include "utils.h"
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
            error("bad output disk config");
            return std::unique_ptr<FluxSink>();
    }
}
