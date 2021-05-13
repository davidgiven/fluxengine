#include "globals.h"
#include "flags.h"
#include "dataspec.h"
#include "sector.h"
#include "sectorset.h"
#include "imagereader/imagereader.h"
#include "utils.h"
#include "fmt/format.h"
#include "lib/config.pb.h"
#include <algorithm>
#include <ctype.h>

std::unique_ptr<ImageReader> ImageReader::create(const InputFileProto& config)
{
	if (config.has_img())
		return ImageReader::createImgImageReader(config);
	else
		Error() << "bad input file config";
	return std::unique_ptr<ImageReader>();
}

ImageReader::ImageReader(const InputFileProto& config):
    _config(config)
{}

