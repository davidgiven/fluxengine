#include "globals.h"
#include "fluxmap.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
#include "arch/agat/agat.h"
#include "arch/amiga/amiga.h"
#include "arch/apple2/apple2.h"
#include "arch/brother/brother.h"
#include "arch/c64/c64.h"
#include "arch/ibm/ibm.h"
#include "arch/macintosh/macintosh.h"
#include "arch/micropolis/micropolis.h"
#include "arch/northstar/northstar.h"
#include "arch/tids990/tids990.h"
#include "arch/victor9k/victor9k.h"
#include "lib/encoders/encoders.pb.h"
#include "lib/proto.h"
#include "lib/layout.h"
#include "lib/image.h"
#include "protocol.h"

std::unique_ptr<Encoder> Encoder::create(const EncoderProto& config)
{
    static const std::map<int,
        std::function<std::unique_ptr<Encoder>(const EncoderProto&)>>
        encoders = {
            {EncoderProto::kAmiga,      createAmigaEncoder      },
            {EncoderProto::kAgat,       createAgatEncoder       },
            {EncoderProto::kApple2,     createApple2Encoder     },
            {EncoderProto::kBrother,    createBrotherEncoder    },
            {EncoderProto::kC64,        createCommodore64Encoder},
            {EncoderProto::kIbm,        createIbmEncoder        },
            {EncoderProto::kMacintosh,  createMacintoshEncoder  },
            {EncoderProto::kMicropolis, createMicropolisEncoder },
            {EncoderProto::kNorthstar,  createNorthstarEncoder  },
            {EncoderProto::kTids990,    createTids990Encoder    },
            {EncoderProto::kVictor9K,   createVictor9kEncoder   },
    };

    auto encoder = encoders.find(config.format_case());
    if (encoder == encoders.end())
        error("no encoder specified");

    return (encoder->second)(config);
}

nanoseconds_t Encoder::calculatePhysicalClockPeriod(
    nanoseconds_t targetClockPeriod, nanoseconds_t targetRotationalPeriod)
{
    nanoseconds_t currentRotationalPeriod =
        globalConfig().drive().rotational_period_ms() * 1e6;
    if (currentRotationalPeriod == 0)
        error(
            "you must set --drive.rotational_period_ms as it can't be "
            "autodetected");

    return targetClockPeriod *
           (currentRotationalPeriod / targetRotationalPeriod);
}

std::shared_ptr<const Sector> Encoder::getSector(
    std::shared_ptr<const TrackInfo>& trackInfo,
    const Image& image,
    unsigned sectorId)
{
    return image.get(trackInfo->logicalTrack, trackInfo->logicalSide, sectorId);
}

std::vector<std::shared_ptr<const Sector>> Encoder::collectSectors(
    std::shared_ptr<const TrackInfo>& trackLayout, const Image& image)
{
    std::vector<std::shared_ptr<const Sector>> sectors;

    for (unsigned sectorId : trackLayout->diskSectorOrder)
    {
        const auto& sector = getSector(trackLayout, image, sectorId);
        if (!sector)
            error("sector {}.{}.{} is missing from the image",
                trackLayout->logicalTrack,
                trackLayout->logicalSide,
                sectorId);
        sectors.push_back(sector);
    }

    return sectors;
}

Fluxmap& Fluxmap::appendBits(const std::vector<bool>& bits, nanoseconds_t clock)
{
    nanoseconds_t now = duration();
    for (unsigned i = 0; i < bits.size(); i++)
    {
        now += clock;
        if (bits[i])
        {
            unsigned delta = (now - duration()) / NS_PER_TICK;
            appendInterval(delta);
            appendPulse();
        }
    }

    return *this;
}
