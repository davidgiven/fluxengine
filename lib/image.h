#ifndef IMAGE_H
#define IMAGE_H

class SectorSet;
class ImageSpec;

extern void readSectorsFromFile(
	SectorSet& sectors,
	const ImageSpec& filename);

extern void writeSectorsToFile(
	const SectorSet& sectors,
	const ImageSpec& filename);

#endif
