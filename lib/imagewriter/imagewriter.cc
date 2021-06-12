#include "globals.h"
#include "flags.h"
#include "sector.h"
#include "sectorset.h"
#include "imagewriter/imagewriter.h"
#include "utils.h"
#include "lib/config.pb.h"
#include "proto.h"
#include "fmt/format.h"
#include <iostream>
#include <fstream>

std::unique_ptr<ImageWriter> ImageWriter::create(const ImageWriterProto& config)
{
	switch (config.format_case())
	{
		case ImageWriterProto::kImg:
			return ImageWriter::createImgImageWriter(config);

		case ImageWriterProto::kLdbs:
			return ImageWriter::createLDBSImageWriter(config);

#if 0
		case ImageWriterProto::kDiskcopy:
			return ImageWriter::createDiskCopyImageWriter(config);

		case ImageWriterProto::kNsi:
			return ImageWriter::createNsiImageWriter(config);
#endif

		default:
			Error() << "bad output image config";
			return std::unique_ptr<ImageWriter>();
	}
}

void ImageWriter::updateConfigForFilename(ImageWriterProto* proto, const std::string& filename)
{
	static const std::map<std::string, std::function<void(void)>> formats =
	{
		{".adf",      [&]() { proto->mutable_img(); }},
		{".d64",      [&]() { proto->mutable_img(); }},
		{".d81",      [&]() { proto->mutable_img(); }},
		{".diskcopy", [&]() { proto->mutable_diskcopy(); }},
		{".img",      [&]() { proto->mutable_img(); }},
		{".ldbs",     [&]() { proto->mutable_ldbs(); }},
		{".st",       [&]() { proto->mutable_img(); }},
		{".nsi",      [&]() { proto->mutable_nsi(); }},
	};

	for (const auto& it : formats)
	{
		if (endsWith(filename, it.first))
		{
			it.second();
			proto->set_filename(filename);
			return;
		}
	}

	Error() << fmt::format("unrecognised image filename '{}'", filename);
}

ImageWriter::ImageWriter(const ImageWriterProto& config):
	_config(config)
{}

