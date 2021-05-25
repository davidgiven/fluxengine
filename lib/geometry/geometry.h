#ifndef DGMAPPER_H
#define DGMAPPER_H

class Sector;
class GeometryProto;
class ImageReader;

class DisassemblingGeometryMapper
{
public:
	virtual const Sector* get(unsigned cylinder, unsigned head, unsigned sector) const = 0;
};

extern std::unique_ptr<DisassemblingGeometryMapper> createSimpleDisassemblingGeometryMapper(
	const GeometryProto& proto, ImageReader& reader);

#endif

