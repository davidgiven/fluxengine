#include "lib/core/globals.h"
#include "lib/config/flags.h"
#include "lib/data/sector.h"
#include "lib/imagereader/imagereader.h"
#include "lib/data/image.h"
#include "lib/config/proto.h"
#include "lib/core/logger.h"
#include "lib/config/config.pb.h"
#include <algorithm>
#include <iostream>
#include <fstream>

// reader based on this partial documentation of the D88 format:
// https://www.pc98.org/project/doc/d88.html

class D88ImageReader : public ImageReader
{
public:
    D88ImageReader(const ImageReaderProto& config): ImageReader(config) {}

    std::unique_ptr<Image> readImage() override
    {
        std::ifstream inputFile(
            _config.filename(), std::ios::in | std::ios::binary);
        if (!inputFile.is_open())
            error("cannot open input file");

        Bytes header(0x24); // read first entry of track table as well
        inputFile.read((char*)header.begin(), header.size());

        // the DIM header technically has a bit field for sectors present,
        // however it is currently ignored by this reader

        std::string diskName = header.slice(0, 0x16);

        if (diskName[0])
            log("D88: disk name: {}", diskName);

        ByteReader headerReader(header);

        char mediaFlag = headerReader.seek(0x1b).read_8();

        inputFile.seekg(0, std::ios::end);
        int fileSize = inputFile.tellg();

        int diskSize = headerReader.seek(0x1c).read_le32();

        if (diskSize > fileSize)
            log("D88: found multiple disk images. Only using first");

        int trackTableEnd = headerReader.seek(0x20).read_le32();
        int trackTableSize = trackTableEnd - 0x20;

        Bytes trackTable(trackTableSize);
        inputFile.seekg(0x20);
        inputFile.read((char*)trackTable.begin(), trackTable.size());
        ByteReader trackTableReader(trackTable);

        auto ibm = _extraConfig.mutable_encoder()->mutable_ibm();
        int clockRate = 500;
        if (mediaFlag == 0x20)
        {
            _extraConfig.mutable_drive()->set_high_density(true);
            _extraConfig.mutable_layout()->set_format_type(FORMATTYPE_80TRACK);
        }
        else
        {
            clockRate = 300;
            _extraConfig.mutable_drive()->set_high_density(false);
            _extraConfig.mutable_layout()->set_format_type(FORMATTYPE_40TRACK);
        }

        auto layout = _extraConfig.mutable_layout();
        std::unique_ptr<Image> image(new Image);
        for (int track = 0; track < trackTableSize / 4; track++)
        {
            int trackOffset = trackTableReader.seek(track * 4).read_le32();
            if (trackOffset == 0)
                continue;

            int currentTrackOffset = trackOffset;
            int currentTrackTrack = -1;
            int currentSectorsInTrack =
                0xffff; // don't know # of sectors until we read the first one
            int trackSectorSize = -1;
            int trackMfm = -1;

            auto trackdata = ibm->add_trackdata();
            trackdata->set_target_clock_period_us(1e3 / clockRate);
            trackdata->set_target_rotational_period_ms(167);

            auto layoutdata = layout->add_layoutdata();
            auto physical = layoutdata->mutable_physical();

            for (int sectorInTrack = 0; sectorInTrack < currentSectorsInTrack;
                 sectorInTrack++)
            {
                Bytes sectorHeader(0x10);
                inputFile.read(
                    (char*)sectorHeader.begin(), sectorHeader.size());
                ByteReader sectorHeaderReader(sectorHeader);
                int track = sectorHeaderReader.seek(0).read_8();
                int head = sectorHeaderReader.seek(1).read_8();
                int sectorId = sectorHeaderReader.seek(2).read_8();
                int sectorSize = 128 << sectorHeaderReader.seek(3).read_8();
                int sectorsInTrack = sectorHeaderReader.seek(4).read_le16();
                int fm = sectorHeaderReader.seek(6).read_8();
                int ddam = sectorHeaderReader.seek(7).read_8();
                int fddStatusCode = sectorHeaderReader.seek(8).read_8();
                int rpm = sectorHeaderReader.seek(13).read_8();
                int dataLength = sectorHeaderReader.seek(14).read_le16();
                if (dataLength < sectorSize)
                {
                    dataLength = sectorSize;
                }
                // D88 provides much more sector information that is currently
                // ignored
                if (ddam != 0)
                    error("D88: nonzero ddam currently unsupported");
                if (rpm != 0)
                    error("D88: 1.44MB 300rpm formats currently unsupported");
                if (fddStatusCode != 0)
                    error(
                        "D88: nonzero fdd status codes are currently "
                        "unsupported");
                if (currentSectorsInTrack == 0xffff)
                {
                    currentSectorsInTrack = sectorsInTrack;
                }
                else if (currentSectorsInTrack != sectorsInTrack)
                {
                    error("D88: mismatched number of sectors in track");
                }
                if (currentTrackTrack < 0)
                {
                    currentTrackTrack = track;
                }
                else if (currentTrackTrack != track)
                {
                    error(
                        "D88: all sectors in a track must belong to the same "
                        "track");
                }
                if (trackSectorSize < 0)
                {
                    trackSectorSize = sectorSize;
                    // this is the first sector we've read, use it settings for
                    // per-track data

                    layoutdata->set_track(track);
                    layoutdata->set_side(head);
                    layoutdata->set_sector_size(sectorSize);

                    trackdata->set_track(track);
                    trackdata->set_head(head);
                    trackdata->set_use_fm(fm);
                    if (fm)
                    {
                        trackdata->set_gap_fill_byte(0xffff);
                        trackdata->set_idam_byte(0xf57e);
                        trackdata->set_dam_byte(0xf56f);
                    }
                    // create timings to approximately match N88-BASIC
                    if (clockRate == 300)
                    {
                        if (sectorSize <= 256)
                        {
                            trackdata->set_gap0(0x1b);
                            trackdata->set_gap2(0x14);
                            trackdata->set_gap3(0x1b);
                        }
                    }
                    else
                    {
                        if (sectorSize <= 128)
                        {
                            trackdata->set_gap0(0x1b);
                            trackdata->set_gap2(0x09);
                            trackdata->set_gap3(0x1b);
                        }
                        else if (sectorSize <= 256)
                        {
                            trackdata->set_gap0(0x36);
                            trackdata->set_gap3(0x36);
                        }
                    }
                }
                else if (trackSectorSize != sectorSize)
                {
                    error(
                        "D88: multiple sector sizes per track are currently "
                        "unsupported");
                }
                Bytes data(sectorSize);
                inputFile.read((char*)data.begin(), data.size());
                inputFile.seekg(dataLength - sectorSize, std::ios_base::cur);
                physical->add_sector(sectorId);
                const auto& sector = image->put(track, head, sectorId);
                sector->status = Sector::OK;
                sector->data = data;
            }

            if (mediaFlag != 0x20)
            {
                auto trackdata = ibm->add_trackdata();
                trackdata->set_target_clock_period_us(1e3 / clockRate);
                trackdata->set_target_rotational_period_ms(167);
            }
        }

        image->calculateSize();
        const Geometry& geometry = image->getGeometry();
        log("D88: read {} tracks, {} sides",
            geometry.numTracks,
            geometry.numSides);

        layout->set_tracks(geometry.numTracks);
        layout->set_sides(geometry.numSides);

        return image;
    }
};

std::unique_ptr<ImageReader> ImageReader::createD88ImageReader(
    const ImageReaderProto& config)
{
    return std::unique_ptr<ImageReader>(new D88ImageReader(config));
}
