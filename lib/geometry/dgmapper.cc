#include "globals.h"
#include "imagereader/imagereader.h"
#include "sectorset.h"
#include "sector.h"
#include "geometry.h"

class SimpleDisassemblingGeometryMapper : public DisassemblingGeometryMapper
{
public:
	SimpleDisassemblingGeometryMapper(const GeometryProto& config, ImageReader& reader):
			_config(config),
			_sectors(reader.readImage())
	{
	}

	const Sector* get(unsigned cylinder, unsigned head, unsigned sector) const
	{
		return _sectors.get(cylinder, head, sector);
	}

private:
	const GeometryProto& _config;
	const SectorSet _sectors;
};

std::unique_ptr<DisassemblingGeometryMapper> createSimpleDisassemblingGeometryMapper(
	const GeometryProto& config, ImageReader& reader)
{
	return std::unique_ptr<DisassemblingGeometryMapper>(new SimpleDisassemblingGeometryMapper(config, reader));
}

