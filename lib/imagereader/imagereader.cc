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

		case ImageReaderProto::kNsi:
			return ImageReader::createNsiImageReader(config);

		default:
			Error() << "bad input file config";
			return std::unique_ptr<ImageReader>();
	}
}

void ImageReader::updateConfigForFilename(ImageReaderProto* proto, const std::string& filename)
{
	static const std::map<std::string, std::function<void(void)>> formats =
	{
		{".adf",      [&]() { proto->mutable_img(); }},
		{".jv3",      [&]() { proto->mutable_jv3(); }},
		{".d64",      [&]() { proto->mutable_d64(); }},
		{".d81",      [&]() { proto->mutable_img(); }},
		{".diskcopy", [&]() { proto->mutable_diskcopy(); }},
		{".img",      [&]() { proto->mutable_img(); }},
		{".st",       [&]() { proto->mutable_img(); }},
		{".nsi",      [&]() { proto->mutable_nsi(); }},
		{".vgi",      [&]() { proto->mutable_img(); }},
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

ImageReader::ImageReader(const ImageReaderProto& config):
    _config(config)
{}

void getTrackFormat(const ImgInputOutputProto& config,
		ImgInputOutputProto::TrackdataProto& trackdata, unsigned track, unsigned side)
{
	trackdata.Clear();
	for (const ImgInputOutputProto::TrackdataProto& f : config.trackdata())
	{
		if (f.has_track() && f.has_up_to_track() && ((track < f.track()) || (track > f.up_to_track())))
			continue;
		if (f.has_track() && !f.has_up_to_track() && (track != f.track()))
			continue;
		if (f.has_side() && (f.side() != side))
			continue;

		trackdata.MergeFrom(f);
	}
}

