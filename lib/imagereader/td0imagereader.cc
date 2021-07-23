#include "globals.h"
#include "flags.h"
#include "sector.h"
#include "imagereader/imagereader.h"
#include "image.h"
#include "crc.h"
#include "fmt/format.h"
#include "lib/config.pb.h"
#include "fmt/format.h"
#include <algorithm>
#include <iostream>
#include <fstream>

/* The best description of the Teledisk format I've found is available here:
 *
 * https://web.archive.org/web/20210420230238/http://dunfield.classiccmp.org/img47321/td0notes.txt
 *
 * Header:
 *
 * Signature                   (2 bytes); TD for normal, td for compressed
 * Sequence                    (1 byte)
 * Checksequence               (1 byte)
 * Teledisk version            (1 byte)
 * Data rate                   (1 byte)
 * Drive type                  (1 byte)
 * Stepping                    (1 byte)
 * DOS allocation flag         (1 byte)
 * Sides                       (1 byte)
 * Cyclic Redundancy Check     (2 bytes)
 */

enum
{
	TD0_ENCODING_RAW = 0,
	TD0_ENCODING_REPEATED = 1,
	TD0_ENCODING_RLE = 2,

	TD0_FLAG_DUPLICATE = 0x01,
	TD0_FLAG_CRC_ERROR = 0x02,
	TD0_FLAG_DELETED   = 0x04,
	TD0_FLAG_SKIPPED   = 0x10,
	TD0_FLAG_IDNODATA  = 0x20,
	TD0_FLAG_DATANOID  = 0x40,
};

class Td0ImageReader : public ImageReader
{
public:
	Td0ImageReader(const ImageReaderProto& config):
		ImageReader(config)
	{}

	Image readImage()
	{
        std::ifstream inputFile(_config.filename(), std::ios::in | std::ios::binary);
        if (!inputFile.is_open())
            Error() << "cannot open input file";

		Bytes input;
		input.writer() += inputFile;
		ByteReader br(input);

		uint16_t signature = br.read_be16();
		br.skip(2); /* sequence and checksequence */
		uint8_t version = br.read_8();
		br.skip(2); /* data rate, drive type */
		uint8_t stepping = br.read_8();
		br.skip(1); /* sparse flag */
		uint8_t sides = (br.read_8() == 1) ? 1 : 2;
		uint16_t headerCrc = br.read_le16();

		uint16_t gotCrc = crc16(0xa097, 0, input.slice(0, 10));
		if (gotCrc != headerCrc)
			Error() << "TD0: header checksum mismatch";
		if (signature != 0x5444)
			Error() << "TD0: unsupported file type (only uncompressed files are supported for now)";

		std::string comment = "(no comment)";
		if (stepping & 0x80)
		{
			/* Comment block */

			br.skip(2); /* comment CRC */
			uint16_t length = br.read_le16();
			br.skip(6); /* timestamp */
			comment = br.read(length);
			std::replace(comment.begin(), comment.end(), '\0', '\n');

			/* Strip trailing newlines */

			auto nl = std::find_if(comment.rbegin(), comment.rend(),
					[](unsigned char ch) { return !std::isspace(ch); });
			comment.erase(nl.base(), comment.end());
		}

		std::cout << fmt::format("TD0: TeleDisk {:.1}: {}\n",
			(double)version / 10.0, comment);

		Image image;
		for (;;)
		{
			/* Read track header */

			uint8_t sectorCount = br.read_8();
			if (sectorCount == 0xff)
				break;

			uint8_t physicalCylinder = br.read_8();
			uint8_t physicalHead = br.read_8() & 1;
			br.skip(1); /* crc */

			for (int i = 0; i < sectorCount; i++)
			{
				/* Read sector */

				uint8_t logicalTrack = br.read_8();
				uint8_t logicalSide = br.read_8();
				uint8_t sectorId = br.read_8();
				uint8_t sectorSizeEncoded = br.read_8();
				unsigned sectorSize = 128<<sectorSizeEncoded;
				uint8_t flags = br.read_8();
				br.skip(1); /* CRC */

				uint16_t dataSize = br.read_le16();
				Bytes encodedData = br.read(dataSize);
				ByteReader bre(encodedData);
				uint8_t encoding = bre.read_8();

				Bytes data;
				if (!(flags & (TD0_FLAG_SKIPPED|TD0_FLAG_IDNODATA)))
				{
					switch (encoding)
					{
						case TD0_ENCODING_RAW:
							data = encodedData.slice(1);
							break;

						case TD0_ENCODING_REPEATED:
						{
							ByteWriter bw(data);
							while (!bre.eof())
							{
								uint16_t pattern = bre.read_le16();
								uint16_t count = bre.read_le16();
								while (count--)
									bw.write_le16(pattern);
							}
							break;
						}

						case TD0_ENCODING_RLE:
						{
							ByteWriter bw(data);
							while (!bre.eof())
							{
								uint8_t length = bre.read_8()*2;
								if (length == 0)
								{
									/* Literal block */

									length = bre.read_8();
									bw += bre.read(length);
								}
								else
								{
									/* Repeated block */

									uint8_t count = bre.read_8();
									Bytes b = bre.read(length);
									while (count--)
										bw += b;
								}
							}
							break;
						}
					}
				}

				const auto& sector = image.put(logicalTrack, logicalSide, sectorId);
				sector->status = Sector::OK;
				sector->physicalCylinder = physicalCylinder;
				sector->physicalHead = physicalHead;
				sector->data = data.slice(0, sectorSize);
			}
		}

		//image.calculateSize();
		const Geometry& geometry = image.getGeometry();
        std::cout << fmt::format("TD0: found {} tracks, {} sides, {} kB total\n",
                        geometry.numTracks, geometry.numSides,
						input.size() / 1024);
        return image;
	}
};

std::unique_ptr<ImageReader> ImageReader::createTd0ImageReader(const ImageReaderProto& config)
{
    return std::unique_ptr<ImageReader>(new Td0ImageReader(config));
}



