#include "globals.h"
#include "flags.h"
#include "dataspec.h"
#include "sector.h"
#include "sectorset.h"
#include "imagereader/imagereader.h"
#include "fmt/format.h"
#include "writer.h"
#include "arch/ibm/ibm.h"
#include <algorithm>
#include <iostream>
#include <fstream>

static unsigned int getModulationandSpeed(uint8_t flags, bool *mfm)
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

struct TrackHeader
{
	uint8_t ModeValue;
	uint8_t track;
	uint8_t Head;
	uint8_t numSectors;
	uint8_t SectorSize;
};

static unsigned int getSectorSize(uint8_t flags)
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


class IMDImageReader : public ImageReader
{
public:
	IMDImageReader(const ImageSpec& spec):
		ImageReader(spec)
	{}

	SectorSet readImage()
	/*
	IMAGE FILE FORMAT
	The overall layout of an ImageDisk .IMD image file is:
	IMD v.vv: dd/mm/yyyy hh:mm:ss
	Comment (ASCII only - unlimited size)
	1A byte - ASCII EOF character
	- For each track on the disk:
	1 byte Mode value							see getModulationspeed for definition		
	1 byte Cylinder
	1 byte Head
	1 byte number of sectors in track			
	1 byte sector size							see getsectorsize for definition
	sector numbering map
	sector cylinder map (optional)				definied in high byte of head (since head is 0 or 1)
	sector head map (optional)					definied in high byte of head (since head is 0 or 1)
	sector data records
	<End of file>
	*/
	{
		//Read File
        std::ifstream inputFile(spec.filename, std::ios::in | std::ios::binary);
        if (!inputFile.is_open())
            Error() << "cannot open input file";
		//define some variables
		int startSectorId = 0;
		bool mfm = false;	//define coding just to show in comment for setting the right write parameters
		inputFile.seekg(0, inputFile.end);
		int inputFileSize = inputFile.tellg();	// determine filesize
		inputFile.seekg(0, inputFile.beg);
		Bytes data;
		data.writer() += inputFile;
		ByteReader br(data);
		SectorSet sectors;
		TrackHeader header = {0, 0, 0, 0, 0};

		unsigned n = 0;
		unsigned headerPtr = 0;
		unsigned int Modulation_Speed = 0;
		unsigned int sectorSize = 0;
		std::string sector_skew;
		int b; 	
		unsigned char comment[8192]; //i choose a fixed value. dont know how to make dynamic arrays in C++. This should be enough
		// Read comment
		while ((b = br.read_8()) != EOF && b != END_OF_FILE)
		{
			comment[n++] = (unsigned char)b;
		}
		headerPtr = n; //set pointer to after comment
		comment[n] = '\0'; // null-terminate the string
		//write comment to screen
		std::cout 	<< "Comment in IMD image:\n"
					<< fmt::format("{}\n",
					comment);

		//first read header
		bool blnBaseOne = false; 
		for (;;)
		{
			if (headerPtr >= inputFileSize-1)
			{
				break;
			}
			header.ModeValue = br.read_8();
			headerPtr++;
			Modulation_Speed = getModulationandSpeed(header.ModeValue, &mfm);
			header.track = br.read_8();
			headerPtr++;
			header.Head = br.read_8();
			headerPtr++;
			header.numSectors = br.read_8();
			headerPtr++;
			header.SectorSize = br.read_8();
			headerPtr++;
			sectorSize = getSectorSize(header.SectorSize);

			//Read optional cylinder map To Do

			//Read optional sector head map To Do

			//read sector numbering map
			unsigned int sector_map[header.numSectors];
			sector_skew.clear();
			for (b = 0;  b < header.numSectors; b++)
			{	
				sector_map[b] = br.read_8();
				if (b == 0) //first sector see if base is 0 or 1 Fluxengine wants 0
					{
					if (sector_map[b]==1)
						{
							blnBaseOne = true;
							startSectorId = 1;
						}
					}
				if (blnBaseOne==true)
				{
					sector_map[b] = (sector_map[b]-1);
					startSectorId = 0;
				}
				sector_skew.push_back(sector_map[b] + '0');
				headerPtr++;
			}
			//read the sectors
			for (int s = 0; s < header.numSectors; s++)
			{
				Bytes sectordata;
				std::unique_ptr<Sector>& sector = sectors.get(header.track, header.Head, sector_map[s]);
				sector.reset(new Sector);
				//read the status of the sector
				unsigned int Status_Sector = br.read_8();
				headerPtr++;

				switch (Status_Sector)
				{
					case 0: /* Sector data unavailable - could not be read */

						break;

					case 1: /* Normal data: (Sector Size) bytes follow */
						sectordata = br.read(sectorSize);
						headerPtr += sectorSize;
						sector->data.writer().append(sectordata);

						break;

					case 2: /* Compressed: All bytes in sector have same value (xx) */
						sectordata = br.read(1);
						headerPtr++;
						sector->data.writer().append(sectordata);

						for (int k = 1; k < sectorSize; k++)
						{
							//fill data till sector is full
							sector->data.writer().append(sectordata);
						}

						break;

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
						Error() << fmt::format("don't understand IMD disks with sector status {}", Status_Sector);
				}		
				sector->status = Sector::OK;
				sector->logicalTrack = sector->physicalTrack = header.track;
				sector->logicalSide = sector->physicalSide = header.Head;
				sector->logicalSector = (sector_map[s]);
			}

  		}
		//Write format detected in IMD image to screen to help user set the right write parameters in case of failures
		size_t headSize = header.numSectors * sectorSize;
        size_t trackSize = headSize * (header.Head + 1);

		std::cout 	<< "reading IMD image\n"
					<< fmt::format("{} tracks, {} heads; {}; {} kbps; {} sectoren; sectorsize {}; sectormap {}; {} kB total \n",
					header.track, header.Head + 1,
					mfm ? "MFM" : "FM",
					Modulation_Speed, header.numSectors, sectorSize, sector_skew, (header.track+1) * trackSize / 1024);
		
		//Set the writer settings based on the read IMD image
		std::string writerinput;
		writerinput= fmt::format(":c={}:h={}:s={}:b={}", header.track + 1, header.Head + 1, header.numSectors, sectorSize);
		setWriterDefaultInput(writerinput);
		std::string writerdest;
		writerdest= fmt::format(":d={}:s={}:t=0-{}", "0" , header.Head ? "0-1" : "0", header.track);
		setWriterDefaultDest(writerdest);
		
		//Set de (IBM) parameters from within the class IMD read image
		IbmEncoder::setsectorSize(sectorSize);
		IbmEncoder::setstartSectorId(startSectorId);
		IbmEncoder::setuseFm(!mfm);
		IbmEncoder::setclockRateKhz(Modulation_Speed);
		IbmEncoder::setsectorSkew(fmt::format(sector_skew));

		//set the rest with the deault (IBM) parameters (because no option was set all these are not filled)
		//First check if these settings have been set bij een flag from the user or we will overwrite them
		//if gap3 ==0 then not initialised so initialise the rest

		if (IbmEncoder::getgap3() == 0)
		{
			IbmEncoder::settrackLengthMs(200);
			IbmEncoder::setemitIam(true);
			IbmEncoder::setidamByte(0x5554);
			IbmEncoder::setdamByte(0x5545);
			IbmEncoder::setgap0(80);
			IbmEncoder::setgap1(50);
			IbmEncoder::setgap2(22);
			IbmEncoder::setgap3(80);
			IbmEncoder::setswapSides(false);
		}

		return sectors;
 	}
};

std::unique_ptr<ImageReader> ImageReader::createIMDImageReader(
	const ImageSpec& spec)
{
    return std::unique_ptr<ImageReader>(new IMDImageReader(spec));
}


