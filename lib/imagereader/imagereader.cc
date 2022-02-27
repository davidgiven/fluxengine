#include "globals.h"
#include "flags.h"
#include "sector.h"
#include "imagereader/imagereader.h"
#include "utils.h"
#include "fmt/format.h"
#include "proto.h"
#include "image.h"
#include "lib/config.pb.h"
#include <algorithm>
#include <ctype.h>

std::unique_ptr<ImageReader> ImageReader::create(const ImageReaderProto& config)
{
	switch (config.format_case())
	{
		case ImageReaderProto::kDim:
			return ImageReader::createDimImageReader(config);

		case ImageReaderProto::kD88:
			return ImageReader::createD88ImageReader(config);

		case ImageReaderProto::kFdi:
			return ImageReader::createFdiImageReader(config);

		case ImageReaderProto::kImd:
			return ImageReader::createIMDImageReader(config);

		case ImageReaderProto::kImg:
			return ImageReader::createImgImageReader(config);

		case ImageReaderProto::kDiskcopy:
			return ImageReader::createDiskCopyImageReader(config);

		case ImageReaderProto::kJv3:
			return ImageReader::createJv3ImageReader(config);

		case ImageReaderProto::kD64:
			return ImageReader::createD64ImageReader(config);

		case ImageReaderProto::kNfd:
			return ImageReader::createNFDImageReader(config);

		case ImageReaderProto::kNsi:
			return ImageReader::createNsiImageReader(config);

		case ImageReaderProto::kTd0:
			return ImageReader::createTd0ImageReader(config);

		default:
			Error() << "bad input file config";
			return std::unique_ptr<ImageReader>();
	}
}

void ImageReader::updateConfigForFilename(ImageReaderProto* proto, const std::string& filename)
{
	static const std::map<std::string, std::function<void(ImageReaderProto*)>> formats =
	{
		{".adf",      [](auto* proto) { proto->mutable_img(); }},
		{".d64",      [](auto* proto) { proto->mutable_d64(); }},
		{".d81",      [](auto* proto) { proto->mutable_img(); }},
		{".d88",      [](auto* proto) { proto->mutable_d88(); }},
		{".dim",      [](auto* proto) { proto->mutable_dim(); }},
		{".diskcopy", [](auto* proto) { proto->mutable_diskcopy(); }},
		{".dsk",      [](auto* proto) { proto->mutable_img(); }},
		{".fdi",      [](auto* proto) { proto->mutable_fdi(); }},
		{".imd",      [](auto* proto) { proto->mutable_imd(); }},
		{".img",      [](auto* proto) { proto->mutable_img(); }},
		{".jv3",      [](auto* proto) { proto->mutable_jv3(); }},
		{".nfd",      [](auto* proto) { proto->mutable_nfd(); }},
		{".nsi",      [](auto* proto) { proto->mutable_nsi(); }},
		{".st",       [](auto* proto) { proto->mutable_img(); }},
		{".td0",      [](auto* proto) { proto->mutable_td0(); }},
		{".vgi",      [](auto* proto) { proto->mutable_img(); }},
		{".xdf",      [](auto* proto) { proto->mutable_img(); }},
	};

	for (const auto& it : formats)
	{
		if (endsWith(filename, it.first))
		{
			it.second(proto);
			proto->set_filename(filename);
			return;
		}
	}

	Error() << fmt::format("unrecognised image filename '{}'", filename);
}

ImageReader::ImageReader(const ImageReaderProto& config):
    _config(config)
{}
