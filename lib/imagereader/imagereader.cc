#include "globals.h"
#include "flags.h"
#include "dataspec.h"
#include "sector.h"
#include "sectorset.h"
#include "imagereader/imagereader.h"
#include "utils.h"
#include "fmt/format.h"
#include <algorithm>
#include <ctype.h>

std::map<std::string, ImageReader::Constructor> ImageReader::formats =
{
	{".adf", ImageReader::createImgImageReader},
	{".d81", ImageReader::createImgImageReader},
	{".diskcopy", ImageReader::createDiskCopyImageReader},
	{".img", ImageReader::createImgImageReader},
	{".ima", ImageReader::createImgImageReader},
	{".jv1", ImageReader::createImgImageReader},
	{".jv3", ImageReader::createJv3ImageReader},
	{".st", ImageReader::createImgImageReader},
};

ImageReader::Constructor ImageReader::findConstructor(const ImageSpec& spec)
{
    const auto& filename = spec.filename;

	for (const auto& e : formats)
	{
		if (endsWith(filename, e.first))
			return e.second;
	}

	return NULL;
}

std::unique_ptr<ImageReader> ImageReader::create(const ImageSpec& spec)
{
    verifyImageSpec(spec);
    return findConstructor(spec)(spec);
}

void ImageReader::verifyImageSpec(const ImageSpec& spec)
{
    if (!findConstructor(spec))
        Error() << "unrecognised input image filename extension";
}

ImageReader::ImageReader(const ImageSpec& spec):
    spec(spec)
{}

