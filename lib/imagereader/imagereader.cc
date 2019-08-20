#include "globals.h"
#include "image.h"
#include "flags.h"
#include "dataspec.h"
#include "sector.h"
#include "sectorset.h"
#include "imagereader/imagereader.h"
#include "fmt/format.h"

std::map<std::string, ImageReader::Constructor> ImageReader::formats =
{
	{".adf", ImageReader::createImgImageReader},
	{".d81", ImageReader::createImgImageReader},
	{".img", ImageReader::createImgImageReader},
};

static bool ends_with(const std::string& value, const std::string& ending)
{
    if (ending.size() > value.size())
        return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

ImageReader::Constructor ImageReader::findConstructor(const ImageSpec& spec)
{
    const auto& filename = spec.filename;

	for (const auto& e : formats)
	{
		if (ends_with(filename, e.first))
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
        Error() << "unrecognised image filename extension";
}

ImageReader::ImageReader(const ImageSpec& spec):
    spec(spec)
{}

