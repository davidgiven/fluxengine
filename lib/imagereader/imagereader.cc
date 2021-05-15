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
	switch (config.format_case())
	{
		case InputFileProto::kImd:
			return ImageReader::createIMDImageReader(config);

		case InputFileProto::kImg:
			return ImageReader::createImgImageReader(config);

		case InputFileProto::kDiskcopy:
			return ImageReader::createDiskCopyImageReader(config);

		case InputFileProto::kJv3:
			return ImageReader::createJv3ImageReader(config);
	}

	Error() << "bad input file config";
	return std::unique_ptr<ImageReader>();
}

ImageReader::ImageReader(const InputFileProto& config):
    _config(config)
{}

