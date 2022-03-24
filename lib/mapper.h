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

    static unsigned remapCylinderPhysicalToLogical(unsigned cylinder);
    static unsigned remapCylinderLogicalToPhysical(unsigned cylinder);

	static std::set<Location> computeLocations();
};

#endif
