#include "lib/globals.h"
#include "lib/flags.h"
#include "lib/sector.h"
#include "lib/imagereader/imagereader.h"
#include "lib/image.h"
#include "lib/proto.h"
#include "lib/logger.h"
#include "lib/mapper.h"
#include "lib/config.pb.h"
#include "fmt/format.h"
#include <algorithm>
#include <iostream>
#include <fstream>

// reader based on this partial documentation of the D88 format:
// https://www.pc98.org/project/doc/d88.html

class D88ImageReader : public ImageReader
{
public:
    D88ImageReader(const ImageReaderProto& config): ImageReader(config) {}

    std::unique_ptr<Image> readImage()
    {
        std::ifstream inputFile(
            _config.filename(), std::ios::in | std::ios::binary);
        if (!inputFile.is_open())
            Error() << "cannot open input file";

        Bytes header(0x24); // read first entry of track table as well
        inputFile.read((char*)header.begin(), header.size());

        // the DIM header technically has a bit field for sectors present,
        // however it is currently ignored by this reader

        std::string diskName = header.slice(0, 0x16);

        if (diskName[0])
            Logger() << fmt::format("D88: disk name: {}", diskName);

        ByteReader headerReader(header);

        char mediaFlag = headerReader.seek(0x1b).read_8();

        inputFile.seekg(0, std::ios::end);
        int fileSize = inputFile.tellg();

        int diskSize = headerReader.seek(0x1c).read_le32();

        if (diskSize > fileSize)
            Logger() << "D88: found multiple disk images. Only using first";

        int trackTableEnd = headerReader.seek(0x20).read_le32();
        int trackTableSize = trackTableEnd - 0x20;

        Bytes trackTable(trackTableSize);
        inputFile.seekg(0x20);
        inputFile.read((char*)trackTable.begin(), trackTable.size());
        ByteReader trackTableReader(trackTable);

        if (config.encoder().format_case() !=
            EncoderProto::FormatCase::FORMAT_NOT_SET)
            Logger() << "D88: overriding configured format";

        auto ibm = config.mutable_encoder()->mutable_ibm();
        int clockRate = 500;
        if (mediaFlag == 0x20)
        {
            Logger() << "D88: high density mode";
			if (!config.drive().has_drive())
				config.mutable_drive()->set_high_density(true);
            if (!config.has_tpi())
                config.set_tpi(96);
        }
        else
        {
            Logger() << "D88: single/double density mode";
            clockRate = 300;
			if (!config.drive().has_drive())
				config.mutable_drive()->set_high_density(false);
            if (!config.has_tpi())
                config.set_tpi(48);
        }

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
            auto sectors = trackdata->mutable_sectors();

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
                // D88 provides much more sector information that is currently
                // ignored
                if (ddam != 0)
                    Error() << "D88: nonzero ddam currently unsupported";
                if (rpm != 0)
                    Error()
                        << "D88: 1.44MB 300rpm formats currently unsupported";
                if (fddStatusCode != 0)
                    Error() << "D88: nonzero fdd status codes are currently "
                               "unsupported";
                if (currentSectorsInTrack == 0xffff)
                {
                    currentSectorsInTrack = sectorsInTrack;
                }
                else if (currentSectorsInTrack != sectorsInTrack)
                {
                    Error() << "D88: mismatched number of sectors in track";
                }
                if (currentTrackTrack < 0)
                {
                    currentTrackTrack = track;
                }
                else if (currentTrackTrack != track)
                {
                    Error() << "D88: all sectors in a track must belong to the "
                               "same track";
                }
                if (trackSectorSize < 0)
                {
                    trackSectorSize = sectorSize;
                    // this is the first sector we've read, use it settings for
                    // per-track data
                    trackdata->set_track(track);
                    trackdata->set_head(head);
                    trackdata->set_sector_size(sectorSize);
                    trackdata->set_use_fm(fm);
                    if (fm)
                    {
                        trackdata->set_gap_fill_byte(0xffff);
                        trackdata->set_idam_byte(0xf57e);
                        trackdata->set_dam_byte(0xf56f);
                    }
                    // create timings to approximately match N88-BASIC
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
                else if (trackSectorSize != sectorSize)
                {
                    Error() << "D88: multiple sector sizes per track are "
                               "currently unsupported";
                }
                Bytes data(sectorSize);
                inputFile.read((char*)data.begin(), data.size());
                const auto& sector = image->put(track, head, sectorId);
                sector->status = Sector::OK;
                sector->logicalTrack = track;
                sector->physicalTrack = Mapper::remapTrackLogicalToPhysical(track);
                sector->logicalSide = sector->physicalHead = head;
                sector->logicalSector = sectorId;
                sector->data = data;

                sectors->add_sector(sectorId);
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
        Logger() << fmt::format("D88: read {} tracks, {} sides",
            geometry.numTracks,
            geometry.numSides);

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

std::unique_ptr<ImageReader> ImageReader::createD88ImageReader(
    const ImageReaderProto& config)
{
    return std::unique_ptr<ImageReader>(new D88ImageReader(config));
}
