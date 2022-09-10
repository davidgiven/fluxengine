#include "globals.h"
#include "sector.h"
#include "image.h"
#include "fmt/format.h"
#include "logger.h"
#include "proto.h"
#include "mapper.h"
#include "flux.h"

unsigned Mapper::remapTrackPhysicalToLogical(unsigned ptrack)
{
    return (ptrack - config.drive().head_bias()) / config.drive().head_width();
}

static unsigned getTrackStep()
{
    unsigned track_step =
        (config.tpi() == 0) ? 1 : (config.drive().tpi() / config.tpi());

    if (track_step == 0)
        Error()
            << "this drive can't write this image, because the head is too big";
    return track_step;
}

unsigned Mapper::remapTrackLogicalToPhysical(unsigned ltrack)
{
    return config.drive().head_bias() + ltrack * getTrackStep();
}

std::set<Location> Mapper::computeLocations()
{
    std::set<Location> locations;

    std::set<unsigned> tracks;
    if (config.has_tracks())
    	tracks = iterate(config.tracks());
    else
    	tracks = iterate(0, config.layout().tracks());

    std::set<unsigned> heads;
    if (config.has_heads())
    	heads = iterate(config.heads());
    else
    	heads = iterate(0, config.layout().sides());

    for (unsigned logicalTrack : tracks)
    {
        for (unsigned head : heads)
            locations.insert(computeLocationFor(logicalTrack, head));
    }

    return locations;
}

Location Mapper::computeLocationFor(unsigned logicalTrack, unsigned logicalHead)
{
    if ((logicalTrack < config.layout().tracks()) &&
        (logicalHead < config.layout().sides()))
    {
        unsigned track_step = getTrackStep();
        unsigned physicalTrack =
            config.drive().head_bias() + logicalTrack * track_step;

        return {.physicalTrack = physicalTrack,
            .logicalTrack = logicalTrack,
            .head = logicalHead,
            .groupSize = track_step};
    }

    Error() << fmt::format(
        "track {}.{} is not part of the image", logicalTrack, logicalHead);
}

nanoseconds_t Mapper::calculatePhysicalClockPeriod(
    nanoseconds_t targetClockPeriod, nanoseconds_t targetRotationalPeriod)
{
    nanoseconds_t currentRotationalPeriod =
        config.drive().rotational_period_ms() * 1e6;
    if (currentRotationalPeriod == 0)
        Error() << "you must set --drive.rotational_period_ms as it can't be "
                   "autodetected";

    return targetClockPeriod *
           (currentRotationalPeriod / targetRotationalPeriod);
}
