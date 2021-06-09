#include "globals.h"
#include "flags.h"
#include "sector.h"
#include "sectorset.h"
#include "imagewriter/imagewriter.h"
#include "lib/config.pb.h"
#include "fmt/format.h"
#include <algorithm>
#include <iostream>
#include <fstream>

class ImgImageWriter : public ImageWriter
{
public:
	ImgImageWriter(const ImageWriterProto& config):
		ImageWriter(config)
	{
		_of.open(_config.filename(), std::ios::out | std::ios::trunc | std::ios::binary);
        if (!_of.is_open())
            Error() << "cannot open input file";
	}

	~ImgImageWriter()
	{
		_of.seekp(0, std::ios::end);
        std::cout << fmt::format("IMG: written output image of {} kB total\n",
						_of.tellp() / 1024);
	}

	void putBlock(size_t offset, size_t length, const Bytes& data)
	{
		_of.seekp(offset);
		data.slice(0, length).writeTo(_of);
	}

private:
	std::ofstream _of;
};

std::unique_ptr<ImageWriter> ImageWriter::createImgImageWriter(
	const ImageWriterProto& config)
{
    return std::unique_ptr<ImageWriter>(new ImgImageWriter(config));
}
