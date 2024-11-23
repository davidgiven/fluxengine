#include "lib/core/globals.h"
#include "lib/config/flags.h"
#include "lib/data/sector.h"
#include "lib/imagewriter/imagewriter.h"
#include "lib/external/ldbs.h"
#include "lib/data/image.h"
#include "lib/core/logger.h"
#include "lib/config/config.pb.h"
#include <algorithm>
#include <iostream>
#include <fstream>

class LDBSImageWriter : public ImageWriter
{
public:
    LDBSImageWriter(const ImageWriterProto& config): ImageWriter(config) {}

    void writeImage(const Image& image) override
    {
        LDBS ldbs;

        const Geometry geometry = image.getGeometry();

        log("LDBS: writing {} tracks, {} sides, {} sectors, {} bytes per "
            "sector",
            geometry.numTracks,
            geometry.numSides,
            geometry.numSectors,
            geometry.sectorSize);

        Bytes trackDirectory;
        ByteWriter trackDirectoryWriter(trackDirectory);
        int trackDirectorySize = 0;
        trackDirectoryWriter.write_le16(0);

        LDBSOutputProto::DataRate dataRate = _config.ldbs().data_rate();
        if (dataRate == LDBSOutputProto::RATE_GUESS)
        {
            dataRate = (geometry.numSectors > 10) ? LDBSOutputProto::RATE_HD
                                                  : LDBSOutputProto::RATE_DD;
            if (geometry.sectorSize <= 256)
                dataRate = LDBSOutputProto::RATE_SD;
            log("LDBS: guessing data rate as {}",
                LDBSOutputProto::DataRate_Name(dataRate));
        }

        LDBSOutputProto::RecordingMode recordingMode =
            _config.ldbs().recording_mode();
        if (recordingMode == LDBSOutputProto::RECMODE_GUESS)
        {
            recordingMode = LDBSOutputProto::RECMODE_MFM;
            log("LDBS: guessing recording mode as {}",
                LDBSOutputProto::RecordingMode_Name(recordingMode));
        }

        for (int track = 0; track < geometry.numTracks; track++)
        {
            for (int side = 0; side < geometry.numSides; side++)
            {
                Bytes trackHeader;
                ByteWriter trackHeaderWriter(trackHeader);

                int actualSectors = 0;
                for (int sectorId = 0; sectorId < geometry.numSectors;
                     sectorId++)
                {
                    const auto& sector = image.get(track, side, sectorId);
                    if (sector)
                        actualSectors++;
                }

                trackHeaderWriter.write_le16(
                    0x000C); /* offset of sector sideers */
                trackHeaderWriter.write_le16(
                    0x0012); /* length of each sector descriptor */
                trackHeaderWriter.write_le16(actualSectors);
                trackHeaderWriter.write_8(dataRate);
                trackHeaderWriter.write_8(recordingMode);
                trackHeaderWriter.write_8(0);    /* format gap length */
                trackHeaderWriter.write_8(0);    /* filler byte */
                trackHeaderWriter.write_le16(0); /* approximate track length */

                for (int sectorId = 0; sectorId < geometry.numSectors;
                     sectorId++)
                {
                    const auto& sector = image.get(track, side, sectorId);
                    if (sector)
                    {
                        uint32_t sectorLabel = (('S') << 24) |
                                               ((track & 0xff) << 16) |
                                               (side << 8) | sectorId;
                        uint32_t sectorAddress =
                            ldbs.put(sector->data, sectorLabel);

                        trackHeaderWriter.write_8(track);
                        trackHeaderWriter.write_8(side);
                        trackHeaderWriter.write_8(sectorId);
                        trackHeaderWriter.write_8(0); /* power-of-two size */
                        trackHeaderWriter.write_8(
                            (sector->status == Sector::OK)
                                ? 0x00
                                : 0x20);              /* 8272 status 1 */
                        trackHeaderWriter.write_8(0); /* 8272 status 2 */
                        trackHeaderWriter.write_8(1); /* number of copies */
                        trackHeaderWriter.write_8(0); /* filler byte */
                        trackHeaderWriter.write_le32(sectorAddress);
                        trackHeaderWriter.write_le16(0); /* trailing bytes */
                        trackHeaderWriter.write_le16(
                            0); /* approximate offset */
                        trackHeaderWriter.write_le16(sector->data.size());
                    }
                }

                uint32_t trackLabel = (('T') << 24) | ((track & 0xff) << 16) |
                                      ((track >> 8) << 8) | side;
                uint32_t trackHeaderAddress = ldbs.put(trackHeader, trackLabel);
                trackDirectoryWriter.write_be32(trackLabel);
                trackDirectoryWriter.write_le32(trackHeaderAddress);
                trackDirectorySize++;
            }
        }

        trackDirectoryWriter.seek(0);
        trackDirectoryWriter.write_le16(trackDirectorySize);

        uint32_t trackDirectoryAddress =
            ldbs.put(trackDirectory, LDBS_TRACK_BLOCK);
        Bytes data = ldbs.write(trackDirectoryAddress);
        data.writeToFile(_config.filename());
    }
};

std::unique_ptr<ImageWriter> ImageWriter::createLDBSImageWriter(
    const ImageWriterProto& config)
{
    return std::unique_ptr<ImageWriter>(new LDBSImageWriter(config));
}
