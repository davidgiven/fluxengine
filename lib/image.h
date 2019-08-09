#ifndef IMAGE_H
#define IMAGE_H

class SectorSet;
class ImageSpec;

extern SectorSet readSectorsFromFile(
	const ImageSpec& filename);

extern void writeSectorsToFile(
	const SectorSet& sectors,
	const ImageSpec& filename);

#endif
