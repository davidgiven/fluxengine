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

#define SECTOR_SIZE 512
#define TAG_SIZE 12

class DiskCopyImageReader : public ImageReader
{
public:
	DiskCopyImageReader(const ImageReaderProto& config):
		ImageReader(config)
	{
		_if.open(_config.filename(), std::ios::in | std::ios::binary);
        if (!_if.is_open())
            Error() << "cannot open input file";

		Bytes header(_if, 0x54);
		ByteReader br(header);

		uint8_t labelLen = br.read_8();
		std::string label = br.read(labelLen);

		br.seek(0x40);
		uint32_t dataSize = br.read_be32();
		uint32_t tagSize = br.read_be32();
		_numSectors = dataSize / SECTOR_SIZE;
		_sectorOffset = 0x54;
		_tagOffset = _sectorOffset + dataSize;

		_if.seekg(0, std::ios::end);
        std::cout << fmt::format("DISKCOPY: reading input image '{}' of {} sectors",
				label, _numSectors);
	}

	Bytes getBlock(size_t offset, size_t length) const
	{
		if ((length != SECTOR_SIZE) && (length != (SECTOR_SIZE+TAG_SIZE)))
			Error() << fmt::format("diskcopy files only support sector lengths of {} and {}\n",
				SECTOR_SIZE, (SECTOR_SIZE+TAG_SIZE));

		unsigned sectorNum = offset / length;
		if (offset % length)
			Error() << fmt::format("unaligned sector read");

		_if.seekg(_sectorOffset + SECTOR_SIZE*sectorNum);
		Bytes data(_if, SECTOR_SIZE);

		if (length != SECTOR_SIZE)
		{
			_if.seekg(_tagOffset + TAG_SIZE*sectorNum);
			data.writer().seekToEnd().append(_if, TAG_SIZE);
		}

		return data;
	}

private:
	mutable std::ifstream _if;
	unsigned _numSectors;
	off_t _sectorOffset;
	off_t _tagOffset;
};

std::unique_ptr<ImageReader> ImageReader::createDiskCopyImageReader(
	const ImageReaderProto& config)
{
    return std::unique_ptr<ImageReader>(new DiskCopyImageReader(config));
}


