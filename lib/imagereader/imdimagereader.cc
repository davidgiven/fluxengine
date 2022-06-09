#include "globals.h"
#include "flags.h"
#include "sector.h"
#include "imagereader/imagereader.h"
#include "image.h"
#include "proto.h"
#include "logger.h"
#include "mapper.h"
#include "lib/config.pb.h"
#include "fmt/format.h"
#include <algorithm>
#include <iostream>
#include <fstream>

static unsigned getModulationandSpeed(uint8_t flags, bool *fm)
{
    switch (flags)
        {
            case 0: /* 500 kbps FM */
                //clockRateKhz.setDefaultValue(250);
                *fm = true;
                return 500;
                break;

            case 1: /* 300 kbps FM */
                *fm  = true;
                return 300;
                
                break;

            case 2: /* 250 kbps FM */
                *fm  = true;
                return 250;
                break;

            case 3: /* 500 kbps MFM */
                *fm  = false;
                return 500;
                break;

            case 4: /* 300 kbps MFM */
                *fm  = false;
                return 300;
                break;

            case 5: /* 250 kbps MFM */
                *fm  = false;
                return 250;
                break;

            default:
                Error() << fmt::format("don't understand IMD disks with this modulation and speed {}", flags);
                throw 0;
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

static std::string copytostring(std::vector<char> tocopy)
{
	std::string result;
	 for (int i = 0; i< tocopy.size(); i++)
	 {

	 	result.push_back(tocopy[i]);
	 }
	return result;
}

static uint8_t setsectorskew(int sectornumber)
{//IMD lets the sector map start with 1 so everything we read in the sectormap has to be 1 less for fluxengine
	sectornumber--;
	switch (sectornumber)
	{
		case 0:
			return '0';
			break;
		case 1:
			return '1';
			break;
		case 2:
			return '2';
			break;
		case 3:
			return '3';
			break;
		case 4:
			return '4';
			break;
		case 5:
			return '5';
			break;
		case 6:
			return '6';
			break;
		case 7:
			return '7';
			break;
		case 8:
			return '8';
			break;
		case 9:
			return '9';
			break;
		case 10:
			return 'a';
			break;
		case 11:
			return 'b';
			break;
		case 12:
			return 'c';
			break;
		case 13:
			return 'd';
			break;
		case 14:
			return 'e';
			break;
		case 15:
			return 'f';
			break;
		case 16:
			return 'g';
			break;
		case 17:
			return 'h';
			break;
	}
	Error() << fmt::format("Sector skew {} not in standard range (0-17).", sectornumber);
}

static unsigned getSectorSize(uint8_t flags)
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
    IMDImageReader(const ImageReaderProto& config): ImageReader(config) {}

    std::unique_ptr<Image> readImage()
    /*
    IMAGE FILE FORMAT
    The overall layout of an ImageDisk .IMD image file is:
    IMD v.vv: dd/mm/yyyy hh:mm:ss
    Comment (ASCII only - unlimited size)
    1A byte - ASCII EOF character
    - For each track on the disk:
    1 byte Mode value                           see getModulationspeed for definition       
    1 byte Track
    1 byte Head
    1 byte number of sectors in track           
    1 byte sector size                          see getsectorsize for definition
    sector numbering map
    sector track map (optional)              definied in high byte of head (since head is 0 or 1)
    sector head map (optional)                  definied in high byte of head (since head is 0 or 1)
    sector data records
    <End of file>
    */
    {
        //Read File
        std::ifstream inputFile(_config.filename(), std::ios::in | std::ios::binary);
        if (!inputFile.is_open())
            Error() << "cannot open input file";
        //define some variables
        bool fm = false;   //define coding just to show in comment for setting the right write parameters
        inputFile.seekg(0, inputFile.end);
        int inputFileSize = inputFile.tellg();  // determine filesize
        inputFile.seekg(0, inputFile.beg);
        Bytes data;
        data.writer() += inputFile;
        ByteReader br(data);
        std::unique_ptr<Image> image(new Image);
        TrackHeader header = {0, 0, 0, 0, 0};
        TrackHeader previousheader = {0, 0, 0, 0, 0};

        unsigned n = 0;
        unsigned headerPtr = 0;
        unsigned Modulation_Speed = 0;
        unsigned sectorSize = 0;
        //std::string sector_skew;
        std::vector<char> sector_skew;
   		std::vector<char> previous_sector_skew;
        sector_skew.clear();

        int b;  
        std::string comment;
		bool blnOptionalCylinderMap = false;
		bool blnOptionalHeadMap = false;

 //       unsigned char comment[8192]; //i choose a fixed value. dont know how to make dynamic arrays in C++. This should be enough
		// Read comment
		comment.clear();
		while ((b = br.read_8()) != EOF && b != END_OF_FILE)
		{
			comment.push_back(b);
			n++;
		}        headerPtr = n; //set pointer to after comment
//        comment[n] = '\0'; // null-terminate the string
        //write comment to screen
//        Logger()   << fmt::format("IMD: comment: {}", comment);
		Logger() 	<< "Comment in IMD file:"
					<< fmt::format("{}",
					comment);

		bool blnBaseOne = false; 

        for (;;)
        {
            if (headerPtr >= inputFileSize-1)
            {
                break;
            }
            //first read header
            header.ModeValue = br.read_8();
            headerPtr++;
            Modulation_Speed = getModulationandSpeed(header.ModeValue, &fm);
            header.track = br.read_8();
            headerPtr++;
            header.Head = br.read_8();
            headerPtr++;
            header.numSectors = br.read_8();
            headerPtr++;
            header.SectorSize = br.read_8();
            headerPtr++;
            sectorSize = getSectorSize(header.SectorSize);

			unsigned optionalsector_map[header.numSectors];
			//The Sector Cylinder Map has one entry for each sector, and contains the logical Cylinder ID for the corresponding sector in the Sector Numbering Map.
			if (header.Head & SEC_CYL_MAP_FLAG) 
			{
				//Read optional cylinder map
				for (b = 0;  b < header.numSectors; b++)
				{
					optionalsector_map[b] = br.read_8();
					headerPtr++;
				}
				blnOptionalCylinderMap = true;				//set bool so we know there is an optional cylinder map
				header.Head = header.Head^SEC_CYL_MAP_FLAG; //remove flag 10000001 ^ 10000000 = 00000001 and 10000000 ^ 10000000 = 00000000 
			}
			//Read optional sector head map
			//The Sector Head Map has one entry for each sector, and contains the logical Head ID for the corresponding sector in the Sector Numbering Map.
			unsigned optionalhead_map[header.numSectors];
			if (header.Head & SEC_HEAD_MAP_FLAG) 
			{
				//Read optional sector head map 
				for (b = 0;  b < header.numSectors; b++)
				{
					optionalhead_map[b] = br.read_8();
					headerPtr++;
				}
				blnOptionalHeadMap = true;					//set bool so we know there is an optional head map
				header.Head = header.Head^SEC_HEAD_MAP_FLAG; //remove flag 01000001 ^ 01000001 = 00000001 and 01000000 ^ 0100000 = 00000000 for writing sector head later
			}            
            //read sector numbering map
            std::vector<unsigned> sector_map(header.numSectors);
            sector_skew.clear();
			for (b = 0;  b < header.numSectors; b++)
			{	
				uint8_t t;
				t = br.read_8();
				headerPtr++;
				sector_map[b] = b+1;	//fluxengine always starts with 1??
				sector_skew.push_back(setsectorskew(t));
            }
            auto ibm = config.mutable_encoder()->mutable_ibm();
           
 			// if (!config.drive().has_rotational_period_ms())
    		// 	config.mutable_drive()->set_rotational_period_ms(200);
    
            auto trackdata = ibm->add_trackdata();
            trackdata->set_target_clock_period_us(1e3 / Modulation_Speed);
            trackdata->set_target_rotational_period_ms(200);

            trackdata->set_use_fm(fm);

            auto sectors = trackdata->mutable_sectors();
            
            //read the sectors
            for (int s = 0; s < header.numSectors; s++)
            {
                Bytes sectordata;
   				Bytes compressed(sectorSize);

                const auto& sector = image->put(header.track, header.Head, sector_map[s]);
                //read the status of the sector
                unsigned int Status_Sector = br.read_8();
                headerPtr++;

                switch (Status_Sector)
                {
				/*fluxengine knows of a few sector statussen but not all of the statussen in IMD.
				*  // the statussen are in sector.h. Translation to fluxengine is as follows:
				*	Statussen fluxengine							|	Status IMD		
				*--------------------------------------------------------------------------------------------------------------------
				*  	OK,												|	1, 2 (Normal data: (Sector Size) of (compressed) bytes follow)
				*	BAD_CHECKSUM,									|	5, 6, 7, 8
				*	MISSING,	  sector not found					|	0 (Sector data unavailable - could not be read)
				*	DATA_MISSING, sector present but no data found	|	3, 4
				*	CONFLICT,										|
				*	INTERNAL_ERROR									|
				*/
					case 0: /* Sector data unavailable - could not be read */

						sector->status = Sector::MISSING;
						break;

					case 1: /* Normal data: (Sector Size) bytes follow */
						sectordata = br.read(sectorSize);
						headerPtr += sectorSize;
						sector->data.writer().append(sectordata);
						sector->status = Sector::OK;
						break;

					case 2: /* Compressed: All bytes in sector have same value (xx) */
						compressed[0] = br.read_8();
						headerPtr++;
						for (int k = 1; k < sectorSize; k++)
						{
							//fill data till sector is full
							br.seek(headerPtr);
							compressed[k] = br.read_8();
						}
						sector->data.writer().append(compressed);
						sector->status = Sector::OK;
						break;

					case 3: /* Normal data with "Deleted-Data address mark" */
						sector->status = Sector::DATA_MISSING;
						sectordata = br.read(sectorSize);
						headerPtr += sectorSize;
						sector->data.writer().append(sectordata);						
						break;

					case 4: /* Compressed with "Deleted-Data address mark"*/
						sector->status = Sector::DATA_MISSING;
						sectordata = br.read(sectorSize);
						headerPtr += sectorSize;
						sector->data.writer().append(sectordata);						
						break;

					case 5: /* Normal data read with data error*/
						sectordata = br.read(sectorSize);
						headerPtr += sectorSize;
						sector->status = Sector::BAD_CHECKSUM;
						sector->data.writer().append(sectordata);						
						break;

					case 6: /* Compressed read with data error*/
						sectordata = br.read(sectorSize);
						headerPtr += sectorSize;
						sector->status = Sector::BAD_CHECKSUM;
						sector->data.writer().append(sectordata);						
						break;

					case 7: /* Deleted data read with data error*/
						sectordata = br.read(sectorSize);
						headerPtr += sectorSize;
						sector->status = Sector::BAD_CHECKSUM;
						sector->data.writer().append(sectordata);						
						break;

					case 8: /* Compressed, Deleted read with data error*/
						sectordata = br.read(sectorSize);
						headerPtr += sectorSize;
						sector->status = Sector::BAD_CHECKSUM;
						sector->data.writer().append(sectordata);						
						break;

					default:
						Error() << fmt::format("Don't understand IMD files with sector status {}, track {}, sector {}", Status_Sector, header.track, s);
				}		
				if (blnOptionalCylinderMap) //there was een optional cylinder map. write it to the sector
				//The Sector Cylinder Map has one entry for each sector, and contains the logical Cylinder ID for the corresponding sector in the Sector Numbering Map.
				{
					sector->physicalTrack = Mapper::remapTrackLogicalToPhysical(header.track);
					//sector->physicalTrack = header.track;
					sector->logicalTrack = optionalsector_map[s];
					blnOptionalCylinderMap = false;
				}
				else 
				{

					//sector->logicalTrack = sector->physicalTrack = header.track;
					sector->logicalTrack = header.track;
                    sector->physicalTrack = Mapper::remapTrackLogicalToPhysical(header.track);
				}
				if (blnOptionalHeadMap) //there was een optional head map. write it to the sector
				//The Sector Head Map has one entry for each sector, and contains the logical Head ID for the corresponding sector in the Sector Numbering Map.
				{
					sector->physicalHead = header.Head;
					sector->logicalSide = optionalhead_map[s];
					blnOptionalHeadMap = false;
				}
				else 
				{
					sector->logicalSide = header.Head;
                    sector->physicalHead = header.Head;
				}
				sector->logicalSector = (sector_map[s]);
            }

        }
        
        if (config.encoder().format_case() !=
            EncoderProto::FormatCase::FORMAT_NOT_SET)
            Logger() << "IMD: overriding configured format";


        image->calculateSize();
        const Geometry& geometry = image->getGeometry();
       	size_t headSize = ((header.numSectors) * (sectorSize));
    	size_t trackSize = (headSize * (header.Head + 1));


    	Logger() << "IMD: read "
				<< fmt::format("{} tracks, {} heads; {}; {} kbps; {} sectoren; sectorsize {}; sectormap {}; {} kB total.",
				header.track + 1, header.Head + 1,
				fm ? "FM" : "MFM",
				Modulation_Speed, header.numSectors, sectorSize, copytostring(sector_skew), (header.track+1) * trackSize / 1024);

        if (!config.has_heads())
        {
            auto* heads = config.mutable_heads();
            heads->set_start(0);
            heads->set_end(geometry.numSides - 1);
        }

        if (!config.has_tracks())
        {
            auto* tracks = config.mutable_tracks();
            tracks->set_start(0);
            tracks->set_end(geometry.numTracks - 1);
        }

        return image;

    }
};

std::unique_ptr<ImageReader> ImageReader::createIMDImageReader(
    const ImageReaderProto& config)
{
    return std::unique_ptr<ImageReader>(new IMDImageReader(config));
}

// vim: ts=4 sw=4 et
