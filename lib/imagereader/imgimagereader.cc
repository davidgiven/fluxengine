#include "globals.h"
#include "flags.h"
#include "sector.h"
#include "sectorset.h"
#include "imagereader/imagereader.h"
#include "lib/config.pb.h"
#include "fmt/format.h"
#include <algorithm>
#include <iostream>
#include <fstream>

class ImgImageReader : public ImageReader
{
public:
	ImgImageReader(const ImageReaderProto& config):
		ImageReader(config)
	{
		_if.open(_config.filename(), std::ios::in | std::ios::binary);
        if (!_if.is_open())
            Error() << "cannot open input file";

		_if.seekg(0, std::ios::end);
        std::cout << fmt::format("IMG: reading input image of {} kB total\n",
						_if.tellg() / 1024);
	}

	Bytes getBlock(size_t offset, size_t length) const
	{
		_if.seekg(offset);
		return Bytes(_if, length);
	}

private:
	mutable std::ifstream _if;
};

std::unique_ptr<ImageReader> ImageReader::createImgImageReader(
	const ImageReaderProto& config)
{
    return std::unique_ptr<ImageReader>(new ImgImageReader(config));
}

