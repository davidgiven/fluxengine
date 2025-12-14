#include "lib/core/globals.h"
#include "lib/config/flags.h"
#include "lib/config/config.h"
#include "lib/fluxsink/fluxsink.h"
#include "lib/config/config.pb.h"
#include "lib/config/proto.h"
#include "lib/core/utils.h"
#include <regex>

std::unique_ptr<FluxSinkFactory> FluxSinkFactory::create(Config& config)
{
    if (!config.hasFluxSink())
        error("no flux sink configured");
    return create(config->flux_sink());
}

std::unique_ptr<FluxSinkFactory> FluxSinkFactory::create(
    const FluxSinkProto& config)
{
    switch (config.type())
    {
        case FLUXTYPE_DRIVE:
            return createHardwareFluxSinkFactory(config.drive());

        case FLUXTYPE_A2R:
            return createA2RFluxSinkFactory(config.a2r());

        case FLUXTYPE_AU:
            return createAuFluxSinkFactory(config.au());

        case FLUXTYPE_VCD:
            return createVcdFluxSinkFactory(config.vcd());

        case FLUXTYPE_SCP:
            return createScpFluxSinkFactory(config.scp());

        case FLUXTYPE_FLUX:
            return createFl2FluxSinkFactory(config.fl2());

        default:
            return std::unique_ptr<FluxSinkFactory>();
    }
}
