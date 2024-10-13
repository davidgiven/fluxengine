#include "lib/core/globals.h"
#include "lib/config/flags.h"
#include "lib/data/sector.h"
#include "lib/imagewriter/imagewriter.h"
#include "lib/data/image.h"
#include "lib/config/config.pb.h"
#include "lib/data/layout.h"
#include "lib/core/logger.h"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>

/*
 * Where to get the type of encoding FM or MFM? Now solved with options in
 * proto config
 */
static const char LABEL[] = "IMD archive by fluxengine on"; // 22 karakters
static uint8_t getModulationandSpeed(
    int flags, ImdOutputProto::RecordingMode mode)
{
    if (flags == 0)
    {
        error(
            "Can't write IMD files with this speed {}, and modulation {}. Did "
            "you read a real disk?",
            flags,
            false);
    }
    else
    {
        flags = 1000000.0 / flags;
    }

    if ((flags > 950) and
        (flags < 1050)) // HD disk 5% discrepency is ok 1000*5% = 50 1 us
    {
        /* 500 kbps */
        if (mode == ImdOutputProto::RECMODE_FM)
        {
            return 0;
        }
        else
        {
            return 3;
        }
    }
    else if ((flags > 1475) and (flags < 1575)) // SD disk
    {
        /* 300 kbps*/
        if (mode == ImdOutputProto::RECMODE_FM)
        {
            return 1;
        }
        else
        {
            return 4;
        }
    }
    else if ((flags > 1900) and (flags < 2100)) // DD disk
    {
        /* 250 kbps */
        if (mode == ImdOutputProto::RECMODE_FM)
        {
            return 2;
        }
        else
        {
            return 5;
        }
    }
    else
    {
        error(
            "IMD: Can't write IMD files with this speed {}, and modulation {}. "
            "Try another format.",
            flags,
            false);
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

static uint8_t setSectorSize(int flags)
{
    switch (flags)
    {
        case 128:
            return 0;
        case 256:
            return 1;
        case 512:
            return 2;
        case 1024:
            return 3;
        case 2048:
            return 4;
        case 4096:
            return 5;
        case 8192:
            return 6;
    }
    error(
        "IMD: Sector size {} not in standard range (128, 256, 512, 1024, 2048, "
        "4096, 8192).",
        flags);
}

#define SEC_CYL_MAP_FLAG 0x80
#define SEC_HEAD_MAP_FLAG 0x40
#define HEAD_MASK 0x3F
#define END_OF_FILE 0x1A

// clang-format off
/*
 * IMAGE FILE FORMAT
 * The overall layout of an ImageDisk .IMD image file is:
 * IMD v.vv: dd/mm/yyyy hh:mm:ss
 * Comment (ASCII only - unlimited size)
 * 1A byte - ASCII EOF character
 * - For each track on the disk:
 * 1 byte Mode value							(0-5) see getModulationspeed for definition		
 * 1 byte Cylinder							(0-n)
 * 1 byte Head								(0-1)
 * 1 byte number of sectors in track			(1-n)
 * 1 byte sector size							(0-6) see getsectorsize for definition
 * sector numbering map						IMD start numbering sectors with 1.
 * sector cylinder map (optional)				definied in high byte of head (since head is 0 or 1)
 * sector head map (optional)					definied in high byte of head (since head is 0 or 1)
 * sector data records	For each data record:
 * 	1 byte Sector status 					
 * 		0: Sector data unavailable - could not be read
 * 		1: Normal data: (Sector Size) bytes follow
 * 		2: Compressed: All bytes in sector have same value (xx)
 * 		3: Normal data with "Deleted-Data address mark"
 * 		4: Compressed with "Deleted-Data address mark"
 * 		5: Normal data read with data error
 * 		6: Compressed read with data error"
 * 		7: Deleted data read with data error"
 * 		8: Compressed, Deleted read with data error"
 * 	sector size of Sector data
 * <End of file>
 */
// clang-format on
class ImdImageWriter : public ImageWriter
{
public:
    ImdImageWriter(const ImageWriterProto& config): ImageWriter(config) {}

    void writeImage(const Image& image) override
    {
        const Geometry& geometry = image.getGeometry();
        unsigned numHeads;
        unsigned numSectors;
        unsigned numBytes;
        std::ofstream outputFile(
            _config.filename(), std::ios::out | std::ios::binary);
        if (!outputFile.is_open())
            error("IMD: cannot open output file");
        unsigned numSectorsinTrack = 0;

        numHeads = geometry.numSides;
        numSectors = geometry.numSectors;
        numBytes = geometry.sectorSize;

        Bytes imagenew;
        ByteWriter bw(imagenew);

        ImdOutputProto::DataRate dataRate = _config.imd().data_rate();
        if (dataRate == ImdOutputProto::RATE_GUESS)
        {
            dataRate = (geometry.numSectors > 10) ? ImdOutputProto::RATE_HD
                                                  : ImdOutputProto::RATE_DD;
            if (geometry.sectorSize <= 256)
                dataRate = ImdOutputProto::RATE_SD;
            log("IMD: guessing data rate as {}",
                ImdOutputProto::DataRate_Name(dataRate));
        }

        ImdOutputProto::RecordingMode recordingMode =
            _config.imd().recording_mode();
        if (recordingMode == ImdOutputProto::RECMODE_GUESS)
        {
            recordingMode = ImdOutputProto::RECMODE_MFM;
            log("IMD: guessing recording mode as {}",
                ImdOutputProto::RecordingMode_Name(recordingMode));
        }

        // Give the user a option to give a comment in the IMD file for archive
        // purposes.
        auto start = std::chrono::system_clock::now();
        std::time_t time = std::chrono::system_clock::to_time_t(start);

        std::string comment = _config.imd().comment();
        if (comment.size() == 0)
        {
            comment = LABEL;
            comment.append(" date: ");
            comment.append(std::ctime(&time));
        }
        else
        {
            comment.insert(0, "IMD ");
        }
        bw.seek(0);

        bw.append(comment);
        bw.write_8(END_OF_FILE);
        std::string sector_skew;
        sector_skew.clear();
        unsigned Status_Sector = 1;
        bool blnOptionalCylinderMap = false;
        bool blnOptionalHeadMap = false;

        /* Write the actual sector data. */
        for (int track = 0; track < geometry.numTracks; track++)
        {
            for (int head = 0; head < numHeads; head++)
            {
                unsigned sectorIdBase = 1; // IMD starts sector numbering with
                                           // 1;
                unsigned sectorId = 0;
                TrackHeader header = {0,
                    0,
                    0,
                    0,
                    0}; // define something to hold the header values
                const auto& sector = image.get(track, head, sectorId + 1);
                if (!sector)
                { // sector 0 doesnt exist exit with error
                    // this track, head has no sectors
                    Status_Sector = 0;
                    log("IMD: sector {} not found on track {}, head {}\n",
                        sectorId + 1,
                        track,
                        head);
                    break;
                }
                else
                {
                    /* Get the header information */
                    numBytes =
                        sector->data.size(); // number of bytes can change per
                                             // sector per track
                    header.track = track;
                    header.Head = head;
                    header.SectorSize = setSectorSize(numBytes);
                    sector_skew.clear();
                    numSectorsinTrack = 0;
                    nanoseconds_t RATE = 0;
                    if (sector->clock > 0)
                    {
                        RATE = 1000000.0 / sector->clock;
                    }
                    else
                    {
                        switch (dataRate)
                        {
                            case ImdOutputProto::RATE_HD:
                                RATE = 1000;
                                break;
                            case ImdOutputProto::RATE_SD:
                                RATE = 1500;
                                break;
                            case ImdOutputProto::RATE_DD:
                                RATE = 2000;
                                break;
                            case ImdOutputProto::RATE_GUESS:
                                break;
                        }
                    }
                    header.ModeValue =
                        getModulationandSpeed(RATE, recordingMode);
                }
                // determine number of sectors in track
                for (int sectorId = 0; sectorId < numSectors; sectorId++)
                {
                    const auto& sector = image.get(track, head, sectorId + 1);
                    if (!sector)
                    {
                        break;
                    }
                    else
                    {
                        numSectorsinTrack++;
                    }
                }
                // determine sector skew and if there are optional cylindermaps
                // or headermaps
                for (int sectorId = 0; sectorId < numSectorsinTrack; sectorId++)
                {
                    const auto& sector = image.get(track, head, sectorId + 1);
                    if (!sector)
                    {
                        break;
                    }
                    else
                    {
                        sector_skew.push_back(
                            (sectorId + sectorIdBase) +
                            '0'); // fill sectorskew start with 1
                        if ((sector->physicalTrack) !=
                            (sector->logicalTrack)) // different physicaltrack
                                                    // fromn logicaltrack
                        {
                            blnOptionalCylinderMap = true;
                        }
                        if (sector->logicalSide !=
                            sector->physicalSide) // different physicalside
                                                  // fromn logicalside
                        {
                            blnOptionalHeadMap = true;
                        }
                    }
                }
                bw.write_8(header.ModeValue); // 1 byte ModeValue
                bw.write_8(track);            // 1 byte Cylinder
                // are there optional cylinder or head maps?
                if (blnOptionalCylinderMap)
                {
                    header.Head = header.Head ^
                                  SEC_CYL_MAP_FLAG; // if head was 0 (00000000)
                                                    // it becomes (10000000)
                }
                if (blnOptionalHeadMap)
                {
                    header.Head = header.Head ^
                                  SEC_HEAD_MAP_FLAG; // if head was 1 (00000001)
                                                     // it becomes (01000001)
                }
                bw.write_8(head); // 1 byte Head
                bw.write_8(
                    numSectorsinTrack); // 1 byte number of sectors in track
                bw.write_8(header.SectorSize); // 1 byte sector size
                for (int sectorId = 0; sectorId < numSectorsinTrack; sectorId++)
                {
                    bw.write_8(
                        (sectorId + sectorIdBase)); // sector numbering map
                }
                // Write optional cylinder map
                // The Sector Cylinder Map has one entry for each sector, and
                // contains the logical Cylinder ID for the corresponding sector
                // in the Sector Numbering Map.
                if (blnOptionalCylinderMap)
                {
                    // determine how the optional cylinder map looks like
                    // write the corresponding logical ID
                    for (int sectorId = 0; sectorId < numSectorsinTrack;
                         sectorId++)
                    {
                        // const auto& sector = sectors.get(track, head,
                        // sectorId);
                        bw.write_8(sector->logicalTrack); // 1 byte logical
                                                          // track
                    }
                }

                // Write optional sector head map
                // The Sector Head Map has one entry for each sector, and
                // contains the logical Head ID for the corresponding sector in
                // the Sector Numbering Map.
                if (blnOptionalHeadMap)
                {
                    // determine how the optional head map looks like
                    // write the corresponding logical ID
                    for (int sectorId = 0; sectorId < numSectorsinTrack;
                         sectorId++)
                    {
                        //	const auto& sector = sectors.get(track, head,
                        // sectorId);
                        bw.write_8(sector->logicalSide); // 1 byte logical side
                    }
                }
                // Now read data and write to file
                for (int sectorId = 0; sectorId < numSectorsinTrack; sectorId++)
                {
                    // clang-format off
                    /*	For each data record:
                     *	1 byte Sector status 					
                     *		0: Sector data unavailable - could not be read
                     *		1: Normal data: (Sector Size) bytes follow
                     *		2: Compressed: All bytes in sector have same value (xx)
                     *		3: Normal data with "Deleted-Data address mark"
                     *		4: Compressed with "Deleted-Data address mark"
                     *		5: Normal data read with data error
                     *		6: Compressed read with data error"
                     *		7: Deleted data read with data error"
                     *		8: Compressed, Deleted read with data error"
                     *	sector size of Sector data
                     */
                    // clang-format on
                    // read sector
                    const auto& sector = image.get(track, head, sectorId + 1);
                    bool blnCompressable =
                        false; // Consists the sector of 1 value? if yes then
                               // compresses IMD this to 1 value
                    Bytes sectordata(numBytes); // define the sectordata with
                                                // the size of the sectorsize
                    Bytes compressed(
                        1);       // reserve 1 byte for comressed sectordata
                    uint8_t byte; // value read
                    uint8_t
                        byte_previous; // previous value read (to determine if
                                       // all bytes are equel in this sector)
                    if (!sector)
                    {
                        Status_Sector = 0;
                        break;
                    }
                    else
                    {
                        ByteReader br(sector->data); // read the sector data
                        int i;
                        // determine if all bytes are the same -> compress and
                        // sector status = 2
                        for (i = 0; i < numBytes; i++)
                        {
                            byte = br.read_8();
                            if (i == 0)
                            {
                                byte_previous = byte;
                            }
                            if (byte_previous == byte)
                            {
                                blnCompressable = true;
                            }
                            else
                            {
                                blnCompressable = false;
                                break;
                            }
                        }
                        switch (sector->status)
                        {
                            // clang-format off
                            /* fluxengine knows of a few sector statussen but not all of the statussen in IMD.
							 *  // the statussen are in sector.h. Translation to fluxengine is as follows:
							 *	Statussen fluxengine							|	Status IMD		
							 *--------------------------------------------------------------------------------------------------------------------
							 *  	OK,											|	1, 2 (Normal data: (Sector Size) of (compressed) bytes follow)
							 *	BAD_CHECKSUM,									|	5, 6, 7, 8
							 *	MISSING,	  sector not found					|	0 (Sector data unavailable - could not be read)
							 *	DATA_MISSING, sector present but no data found	|	3, 4
							 *	CONFLICT,										|
							 *	INTERNAL_ERROR									|
							 */
                            // clang-format on
                            case Sector::MISSING: /* Sector data unavailable -
                                                     could not be read */

                                Status_Sector = 0;
                                break;

                            case Sector::OK: /* Normal data: (Sector Size) bytes
                                                follow */
                                if (blnCompressable) // data is compressable
                                {
                                    Status_Sector = 2;
                                }
                                else
                                {
                                    Status_Sector = 1;
                                }
                                break;
                            case Sector::DATA_MISSING:
                                Status_Sector =
                                    3; // we misuse normal data with
                                       // deleted-data addres mark for this
                                       // missing data option
                                break;
                            // IMD recognizes all of these cases. but fluxengine
                            // doesnt support them. case 2: /* Compressed: All
                            // bytes in sector have same value (xx) */ case 3:
                            // /* Normal data with "Deleted-Data address mark"
                            // */ case 4: /* Compressed with "Deleted-Data
                            // address mark"*/
                            case Sector::BAD_CHECKSUM:
                                // case 5: /* Normal data read with data error -
                                // could not be read*/
                                Status_Sector = 5;
                                break;
                                // case 6: /* Compressed read with data error -
                                // could not be read */ case 7: /* Deleted data
                                // read with data error - could not be read */
                                // case 8: /* Compressed, Deleted read with data
                                // error - could not be read */

                            default:
                                error(
                                    "IMD: Don't understand IMD files with "
                                    "sector status {}",
                                    Status_Sector);
                        }
                        bw.write_8(Status_Sector); // 1 byte status sector
                        if (blnCompressable)
                        {
                            bw.write_8(byte);
                            blnCompressable = false;
                        }
                        else
                        {
                            bw.append(sector->data);
                        }
                        numSectors = numSectorsinTrack;
                    }
                    blnOptionalCylinderMap = false;
                    blnOptionalHeadMap = false;
                }
            }
        }
        imagenew.writeTo(outputFile);
        log("IMD: Written {} tracks, {} heads, {} sectors, {} bytes per "
            "sector, {} kB total",
            geometry.numTracks,
            numHeads,
            numSectors,
            numBytes,
            outputFile.tellp() / 1024);
    }
};

std::unique_ptr<ImageWriter> ImageWriter::createImdImageWriter(
    const ImageWriterProto& config)
{
    return std::unique_ptr<ImageWriter>(new ImdImageWriter(config));
}
