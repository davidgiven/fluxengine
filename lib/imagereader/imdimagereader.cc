#include "globals.h"
#include "flags.h"
#include "sector.h"
#include "sectorset.h"
#include "imagereader/imagereader.h"
#include "geometry/geometry.h"
#include "lib/config.pb.h"
#include "fmt/format.h"
#include <algorithm>
#include <iostream>
#include <fstream>

static unsigned getModulationandSpeed(uint8_t flags, bool *mfm)
{
	switch (flags)
		{
			case 0: /* 500 kbps FM */
				//clockRateKhz.setDefaultValue(250);
				*mfm = false;
				return 500;
				break;

			case 1: /* 300 kbps FM */
				*mfm  = false;
				return 300;
				
				break;

			case 2: /* 250 kbps FM */
				*mfm  = false;
				return 250;
				break;

			case 3: /* 500 kbps MFM */
				*mfm  = true;
				return 500;
				break;

			case 4: /* 300 kbps MFM */
				*mfm  = true;
				return 300;
				break;

			case 5: /* 250 kbps MFM */
				*mfm  = true;
				return 250;
				break;

			default:
				Error() << fmt::format("don't understand IMD disks with this modulation and speed {}", flags);
		}
}

struct Trackheader
{
	uint8_t modeValue;
	uint8_t track;
	uint8_t head;
	uint8_t numSectors;
	uint8_t sectorSize;
};

static unsigned getsectorSize(uint8_t flags)
{
	switch (flags)
	{
		case 0: return 128;
		case 1: return 256;
		case 2: return 512;
		case 3: return 1024;
		case 4: return 2048;
		case 5: return 4096;
		case 6: return 8192;
	}
	Error() << "not reachable";
}


#define SEC_CYL_MAP_FLAG  0x80 
#define SEC_HEAD_MAP_FLAG 0x40
#define HEAD_MASK         0x3F
#define END_OF_FILE       0x1A


class IMDImageReader : public ImageReader, DisassemblingGeometryMapper
{
public:
	IMDImageReader(const ImageReaderProto& config):
		ImageReader(config)
	{
		/*
		 * IMAGE FILE FORMAT
		 * The overall layout of an ImageDisk .IMD image file is:
		 * IMD v.vv: dd/mm/yyyy hh:mm:ss
		 * Comment (ASCII only - unlimited size)
		 * 1A byte - ASCII EOF character
		 * - For each track on the disk:
		 * 1 byte Mode value							see getModulationspeed for definition		
		 * 1 byte Cylinder
		 * 1 byte head
		 * 1 byte number of sectors in track			
		 * 1 byte sector size							see getsectorsize for definition
		 * sector numbering map
		 * sector cylinder map (optional)				definied in high byte of head (since head is 0 or 1)
		 * sector head map (optional)					definied in high byte of head (since head is 0 or 1)
		 * sector data records
		 * <End of file>
		 */

        std::ifstream inputFile(_config.filename(), std::ios::in | std::ios::binary);
        if (!inputFile.is_open())
            Error() << "cannot open input file";
		Bytes data;
		data.writer().append(inputFile);
		ByteReader br(data);

		//define some variables
		bool mfm = false;	//define coding just to show in comment for setting the right write parameters
		Trackheader header = {0, 0, 0, 0, 0};

		unsigned n = 0;
		unsigned modulation_speed = 0;
		unsigned sectorSize = 0;
		std::string sector_skew;

		// Read comment
		std::stringstream comment;
		while (!br.eof())
		{
			int b = br.read_8();
			if (b == 0x1a)
				break;
			if (b == '\r')
				continue;
			if (b == '\n')
				b = 32;
			comment << (char) b;
		}
		std::cout << fmt::format("IMD: comment: {}\n", comment.str());

		while (!br.eof())
		{
			header.modeValue = br.read_8();
			modulation_speed = getModulationandSpeed(header.modeValue, &mfm);
			header.track = br.read_8();
			header.head = br.read_8();
			int physicalHead = header.head & 0x3f;
			header.numSectors = br.read_8();
			header.sectorSize = br.read_8();
			sectorSize = getsectorSize(header.sectorSize);

			/* Read optional cylinder map */

			if (header.head & 0x80)
				Error() << "cylinder map";

			/* Read optional sector head map */

			if (header.head & 0x40)
				Error() << "sector head map";

			//read sector numbering map
			unsigned int sector_map[header.numSectors];
			bool blnBaseOne = false; 
			sector_skew.clear();
			for (int b = 0;  b < header.numSectors; b++)
			{	
				sector_map[b] = br.read_8();
				sector_skew.push_back(sector_map[b] + '0');
				if (b == 0) //first sector see if base is 0 or 1 Fluxengine wants 0
				{
					if (sector_map[b]==1)
						blnBaseOne = true;
				}
				if (blnBaseOne==true)
					sector_map[b] = (sector_map[b]-1);
			}
			//read the sectors
			for (int s = 0; s < header.numSectors; s++)
			{
				std::unique_ptr<Sector>& sector = _sectors.get(header.track, physicalHead, sector_map[s]);
				sector.reset(new Sector);
				//read the status of the sector
				unsigned int sector_status = br.read_8();

				switch (sector_status)
				{
					case 0: /* Sector data unavailable - could not be read */
						break;

					case 1: /* Normal data: (Sector Size) bytes follow */
					{
						Bytes sectordata = br.read(sectorSize);
						sector->data = sectordata;
						break;
					}

					case 2: /* Compressed: All bytes in sector have same value (xx) */
					{
						uint8_t sectordata = br.read_8();
						sector->data.writer().append(sectordata);

						ByteWriter bw(sector->data);
						for (int k = 1; k < sectorSize; k++)
						{
							//fill data till sector is full
							bw.write_8(sectordata);
						}
						break;
					}

					case 3: /* Normal data with "Deleted-Data address mark" */
						break;

					case 4: /* Compressed with "Deleted-Data address mark"*/
						break;

					case 5: /* Normal data read with data error */
						break;

					case 6: /* Compressed read with data error" */
						break;

					case 7: /* Deleted data read with data error" */
						break;

					case 8: /* Compressed, Deleted read with data error" */
						break;

					default:
						Error() << fmt::format("don't understand IMD disks with sector status {}", sector_status);
				}		
				sector->status = Sector::OK;
				sector->logicalTrack = sector->physicalTrack = header.track;
				sector->logicalSide = sector->physicalSide = physicalHead;
				sector->logicalSector = (sector_map[s]);
			}

  		}
		//Write format detected in IMD image to screen to help user set the right write parameters

		size_t headSize = header.numSectors * sectorSize;
        size_t trackSize = headSize * (header.head + 1);

		std::cout << fmt::format("IMD: reading image with {} tracks, {} heads, {};\n"
								 "IMD: {} kbps; {} sectors; sectorsize {}; sectormap {}; {} kB total\n",
				  header.track, header.head + 1,
				  mfm ? "MFM" : "FM",
				  modulation_speed, header.numSectors, sectorSize, sector_skew, (header.track+1) * trackSize / 1024);

	}

	Bytes getBlock(size_t offset, size_t length) const
	{
		throw "unimplemented";
	}

	const Sector* get(unsigned cylinder, unsigned head, unsigned sector) const
	{
		return _sectors.get(cylinder, head, sector);
	}

private:
	SectorSet _sectors;
};

std::unique_ptr<ImageReader> ImageReader::createIMDImageReader(
	const ImageReaderProto& config)
{
    return std::unique_ptr<ImageReader>(new IMDImageReader(config));
}


