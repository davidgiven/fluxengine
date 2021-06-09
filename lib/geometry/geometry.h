#ifndef DGMAPPER_H
#define DGMAPPER_H

class Sector;
class SectorSet;
class GeometryProto;
class ImageReader;
class ImageWriter;

class DisassemblingGeometryMapper
{
public:
	virtual const Sector* get(unsigned cylinder, unsigned head, unsigned sector) const = 0;
};

class AssemblingGeometryMapper
{
public:
	virtual void put(const Sector& sector) const = 0;

public:
	void put(const SectorSet& sectors);
	void printMap(const SectorSet& sectors);
	void writeCsv(const SectorSet& sectors, const std::string& filename);
};

extern std::unique_ptr<DisassemblingGeometryMapper> createSimpleDisassemblingGeometryMapper(
	const GeometryProto& proto, ImageReader& reader);
extern std::unique_ptr<AssemblingGeometryMapper> createSimpleAssemblingGeometryMapper(
	const GeometryProto& proto, ImageWriter& reader);

#endif

