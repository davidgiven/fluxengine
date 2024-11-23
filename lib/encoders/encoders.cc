#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/data/fluxmap.h"
#include "lib/encoders/encoders.h"
#include "lib/encoders/encoders.pb.h"
#include "lib/config/proto.h"
#include "lib/data/layout.h"
#include "lib/data/image.h"
#include "protocol.h"

nanoseconds_t Encoder::calculatePhysicalClockPeriod(
    nanoseconds_t targetClockPeriod, nanoseconds_t targetRotationalPeriod)
{
    nanoseconds_t currentRotationalPeriod =
        globalConfig()->drive().rotational_period_ms() * 1e6;
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
    unsigned delta = (now - duration()) / NS_PER_TICK;
    if (delta)
        appendInterval(delta);

    return *this;
}
