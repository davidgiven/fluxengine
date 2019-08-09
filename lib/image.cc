#include "globals.h"
#include "image.h"
#include "flags.h"
#include "dataspec.h"
#include "sector.h"
#include "sectorset.h"
#include "imagereader/imagereader.h"
#include "imagewriter/imagewriter.h"
#include "fmt/format.h"
#include <algorithm>
#include <iostream>
#include <fstream>

SectorSet readSectorsFromFile(const ImageSpec& spec)
{
	return ImageReader::create(spec)->readImage();
}

void writeSectorsToFile(const SectorSet& sectors, const ImageSpec& spec)
{
	std::unique_ptr<ImageWriter> writer(ImageWriter::create(sectors, spec));
	writer->adjustGeometry();
	writer->printMap();
	writer->writeImage();
}
