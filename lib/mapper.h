#ifndef MAPPER_H
#define MAPPER_H

class SectorMappingProto;
class DriveProto;
class Location;

class Mapper
{
public:
    static std::unique_ptr<const Image> remapSectorsPhysicalToLogical(
        const Image& source, const SectorMappingProto& mapping);
    static std::unique_ptr<const Image> remapSectorsLogicalToPhysical(
        const Image& source, const SectorMappingProto& mapping);

    static unsigned remapTrackPhysicalToLogical(unsigned track);
    static unsigned remapTrackLogicalToPhysical(unsigned track);

    static std::set<Location> computeLocations();

    static nanoseconds_t calculatePhysicalClockPeriod(
        nanoseconds_t targetClockPeriod, nanoseconds_t targetRotationalPeriod);
};

#endif
