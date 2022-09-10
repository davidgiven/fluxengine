#ifndef MAPPER_H
#define MAPPER_H

class SectorMappingProto;
class DriveProto;
class Location;

class Mapper
{
public:
    static unsigned remapTrackPhysicalToLogical(unsigned track);
    static unsigned remapTrackLogicalToPhysical(unsigned track);

    static std::set<Location> computeLocations();
	static Location computeLocationFor(unsigned track, unsigned side);

    static nanoseconds_t calculatePhysicalClockPeriod(
        nanoseconds_t targetClockPeriod, nanoseconds_t targetRotationalPeriod);
};

#endif
