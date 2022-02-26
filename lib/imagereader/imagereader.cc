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

void ImageReader::updateConfigForFilename(
    ImageReaderProto* proto, const std::string& filename)
{
    static const std::map<std::string, std::function<void(void)>> formats = {
        {".adf",
         [&]()
            {
                proto->mutable_img();
            }},
        {".d64",
         [&]()
            {
                proto->mutable_d64();
            }},
        {".d81",
         [&]()
            {
                proto->mutable_img();
            }},
        {".d88",
         [&]()
            {
                proto->mutable_d88();
            }},
        {".dim",
         [&]()
            {
                proto->mutable_dim();
            }},
        {".diskcopy",
         [&]()
            {
                proto->mutable_diskcopy();
            }},
        {".dsk",
         [&]()
            {
                proto->mutable_img();
            }},
        {".fdi",
         [&]()
            {
                proto->mutable_fdi();
            }},
        {".imd",
         [&]()
            {
                proto->mutable_imd();
            }},
        {".img",
         [&]()
            {
                proto->mutable_img();
            }},
        {".jv3",
         [&]()
            {
                proto->mutable_jv3();
            }},
        {".nfd",
         [&]()
            {
                proto->mutable_nfd();
            }},
        {".nsi",
         [&]()
            {
                proto->mutable_nsi();
            }},
        {".st",
         [&]()
            {
                proto->mutable_img();
            }},
        {".td0",
         [&]()
            {
                proto->mutable_td0();
            }},
        {".vgi",
         [&]()
            {
                proto->mutable_img();
            }},
        {".xdf",
         [&]()
            {
                proto->mutable_img();
            }},
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

ImageReader::ImageReader(const ImageReaderProto& config): _config(config) {}
