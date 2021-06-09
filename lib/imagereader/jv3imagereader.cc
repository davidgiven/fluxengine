#include "globals.h"
#include "flags.h"
#include "sector.h"
#include "sectorset.h"
#include "imagereader/imagereader.h"
#include "geometry/geometry.h"
#include "fmt/format.h"
#include "lib/config.pb.h"
#include "lib/imagereader/imagereader.pb.h"
#include <algorithm>
#include <iostream>
#include <fstream>

/* JV3 files are kinda weird. There's a fixed layout for up to 2901 sectors, which may appear
 * in any order, followed by the same again for more sectors. To find the second data block
 * you need to know the size of the first data block, which requires parsing it.
 *
 * https://www.tim-mann.org/trs80/dskconfig.html
 *
 * typedef struct {
 *   SectorHeader headers1[2901];
 *   unsigned char writeprot;
 *   unsigned char data1[];
 *   SectorHeader headers2[2901];
 *   unsigned char padding;
 *   unsigned char data2[];
 * } JV3;
 *
 * typedef struct {
 *   unsigned char track;
 *   unsigned char sector;
 *   unsigned char flags;
 * } SectorHeader;
 */

#define JV3_DENSITY     0x80  /* 1=dden, 0=sden */
#define JV3_DAM         0x60  /* data address mark code; see below */
#define JV3_SIDE        0x10  /* 0=side 0, 1=side 1 */
#define JV3_ERROR       0x08  /* 0=ok, 1=CRC error */
#define JV3_NONIBM      0x04  /* 0=normal, 1=short */
#define JV3_SIZE        0x03  /* in used sectors: 0=256,1=128,2=1024,3=512
                                 in free sectors: 0=512,1=1024,2=128,3=256 */

#define JV3_FREE        0xFF  /* in track and sector fields of free sectors */
#define JV3_FREEF       0xFC  /* in flags field, or'd with size code */

static unsigned getSectorSize(uint8_t flags)
{
	if ((flags & JV3_FREEF) == JV3_FREEF)
	{
		switch (flags & JV3_SIZE)
		{
			case 0: return 512;
			case 1: return 1024;
			case 2: return 128;
			case 3: return 256;
		}
	}
	else
	{
		switch (flags & JV3_SIZE)
		{
			case 0: return 256;
			case 1: return 128;
			case 2: return 1024;
			case 3: return 512;
		}
	}
	Error() << "not reachable";
	throw 0;
}

class Jv3ImageReader : public ImageReader, DisassemblingGeometryMapper
{
public:
	Jv3ImageReader(const ImageReaderProto& config):
		ImageReader(config)
	{
		std::ifstream stream(_config.filename(), std::ios::in | std::ios::binary);
        if (!stream.is_open())
            Error() << "cannot open input file";

		Bytes image;
		image.writer() += stream;
		ByteReader br(image);

		off_t headerPtr = 0;
		for (;;)
		{
			off_t dataPtr = headerPtr + 2901*3 + 1;
			if (dataPtr >= image.size())
				break;

			br.seek(headerPtr);
			for (unsigned i=0; i<2901; i++)
			{
				uint8_t track = br.read_8();
				uint8_t sector = br.read_8();
				uint8_t flags = br.read_8();

				if ((flags & JV3_FREEF) != JV3_FREEF)
				{
					unsigned side = !!(flags & JV3_SIDE);
					unsigned length = getSectorSize(flags);

					std::unique_ptr<Sector> s(new Sector);
					s->status = (flags & JV3_ERROR) ? Sector::BAD_CHECKSUM : Sector::OK;
					s->physicalTrack = s->logicalTrack = track;
					s->physicalSide = s->logicalSide = side;
					s->logicalSector = sector;
					s->data = image.slice(dataPtr, length);
					_sectors[std::make_tuple(track, side, sector)] = std::move(s);
					dataPtr += length;
				}
			}

			/* dataPtr is now pointing at the beginning of the next chunk. */

			headerPtr = dataPtr;
		}

        std::cout << fmt::format("JV3: reading input image of {} sectors total\n",
						_sectors.size());
	}

	const DisassemblingGeometryMapper* getGeometryMapper() const
	{
		return this;
	}

	Bytes getBlock(size_t offset, size_t length) const
	{
		throw "unimplemented";
	}

	const Sector* get(unsigned cylinder, unsigned head, unsigned sector) const
	{
		auto sit = _sectors.find(std::make_tuple(cylinder, head, sector));
		if (sit == _sectors.end())
			return nullptr;

		return sit->second.get();
	}

private:
	std::map<std::tuple<int, int, int>, std::unique_ptr<Sector>> _sectors;
};

std::unique_ptr<ImageReader> ImageReader::createJv3ImageReader(const ImageReaderProto& config)
{
    return std::unique_ptr<ImageReader>(new Jv3ImageReader(config));
}


