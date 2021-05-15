#include "globals.h"
#include "flags.h"
#include "dataspec.h"
#include "sector.h"
#include "sectorset.h"
#include "imagereader/imagereader.h"
#include "utils.h"
#include "fmt/format.h"
#include "proto.h"
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

void ImageReader::updateConfigForFilename(const std::string& filename)
{
	InputFileProto* f = config.mutable_input()->mutable_image();
	static const std::map<std::string, std::function<void(void)>> formats =
	{
		{".adf",      [&]() { f->mutable_img(); }},
		{".jv3",      [&]() { f->mutable_jv3(); }},
		{".d81",      [&]() { f->mutable_img(); }},
		{".diskcopy", [&]() { f->mutable_diskcopy(); }},
		{".img",      [&]() { f->mutable_img(); }},
		{".st",       [&]() { f->mutable_img(); }},
	};

	for (const auto& it : formats)
	{
		if (endsWith(filename, it.first))
		{
			it.second();
			f->set_filename(filename);
			return;
		}
	}

	Error() << fmt::format("unrecognised image filename '{}'", filename);
}

ImageReader::ImageReader(const InputFileProto& config):
    _config(config)
{}

