#include "globals.h"
#include "image.h"
#include "flags.h"
#include "dataspec.h"
#include "sector.h"
#include "sectorset.h"
#include "imagereader/imagereader.h"
#include "fmt/format.h"

static bool ends_with(const std::string& value, const std::string& ending)
{
    if (ending.size() > value.size())
        return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

std::unique_ptr<ImageReader> ImageReader::create(const ImageSpec& spec)
{
    const auto& filename = spec.filename;

    if (ends_with(filename, ".img") || ends_with(filename, ".adf"))
        return createImgImageReader(spec);

    Error() << "unrecognised image filename extension";
    return std::unique_ptr<ImageReader>();
}

ImageReader::ImageReader(const ImageSpec& spec):
    spec(spec)
{}

