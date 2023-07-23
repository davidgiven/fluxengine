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
        case FluxSourceSinkType::DRIVE:
            return createHardwareFluxSink(config.drive());

        case FluxSourceSinkType::A2R:
            return createA2RFluxSink(config.a2r());

        case FluxSourceSinkType::AU:
            return createAuFluxSink(config.au());

        case FluxSourceSinkType::VCD:
            return createVcdFluxSink(config.vcd());

        case FluxSourceSinkType::SCP:
            return createScpFluxSink(config.scp());

        case FluxSourceSinkType::FLUX:
            return createFl2FluxSink(config.fl2());

        default:
            return std::unique_ptr<FluxSink>();
    }
}
