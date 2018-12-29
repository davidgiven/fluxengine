#ifndef IMAGE_H
#define IMAGE_H

class SectorSet;

class Geometry
{
public:
	int tracks;
	int heads;
	int sectors;
	int sectorSize;
};

extern Geometry guessGeometry(const SectorSet& sectors);

extern void writeSectorsToFile(
	const SectorSet& sectors,
	const std::string& filename);

#endif
