#ifndef MAPPER_H
#define MAPPER_H

class SectorMappingProto;

class Mapper
{
public:
	static std::unique_ptr<Image> remapPhysicalToLogical(const Image& source, const SectorMappingProto& mapping);
	static std::unique_ptr<Image> remapLogicalToPhysical(const Image& source, const SectorMappingProto& mapping);
};

#endif

