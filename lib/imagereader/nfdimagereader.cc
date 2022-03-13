#include "globals.h"
#include "flags.h"
#include "sector.h"
#include "imagereader/imagereader.h"
#include "image.h"
#include "proto.h"
#include "logger.h"
#include "lib/config.pb.h"
#include "fmt/format.h"
#include <algorithm>
#include <iostream>
#include <fstream>

// reader based on this partial documentation of the D88 format:
// https://www.pc98.org/project/doc/d88.html

class NFDImageReader : public ImageReader
{
public:
    NFDImageReader(const ImageReaderProto& config): ImageReader(config) {}

    std::unique_ptr<Image> readImage()
    {
        std::ifstream inputFile(
            _config.filename(), std::ios::in | std::ios::binary);
        if (!inputFile.is_open())
            Error() << "cannot open input file";

        Bytes fileId(14); // read first entry of track table as well
        inputFile.read((char*)fileId.begin(), fileId.size());

        if (fileId == Bytes("T98FDDIMAGE.R1"))
        {
            Error() << "NFD: r1 images are not currently supported";
        }
        if (fileId != Bytes("T98FDDIMAGE.R0"))
        {
            Error() << "NFD: could not find NFD header";
        }

        Bytes header(0x10a10);
        inputFile.seekg(0, std::ios::beg);
        inputFile.read((char*)header.begin(), header.size());

        ByteReader headerReader(header);

        char heads = headerReader.seek(0x115).read_8();
        if (heads != 2)
        {
            Error() << "NFD: unsupported number of heads";
        }

        if (config.encoder().format_case() !=
            EncoderProto::FormatCase::FORMAT_NOT_SET)
            Logger() << "NFD: overriding configured format";

        auto ibm = config.mutable_encoder()->mutable_ibm();
        config.mutable_cylinders()->set_end(0);
        Logger() << "NFD: HD 1.2MB mode";
        if (!config.drive().has_drive())
            config.mutable_drive()->set_high_density(true);

        std::unique_ptr<Image> image(new Image);
        for (int track = 0; track < 163; track++)
        {
            auto trackdata = ibm->add_trackdata();
            trackdata->set_clock_rate_khz(500);
            trackdata->set_track_length_ms(167);
            auto sectors = trackdata->mutable_sectors();
            int currentTrackCylinder = -1;
            int currentTrackHead = -1;
            int trackSectorSize = -1;

            for (int sectorInTrack = 0; sectorInTrack < 26; sectorInTrack++)
            {
                headerReader.seek(0x120 + track * 26 * 16 + sectorInTrack * 16);
                int cylinder = headerReader.read_8();
                int head = headerReader.read_8();
                int sectorId = headerReader.read_8();
                int sectorSize = 128 << headerReader.read_8();
                int mfm = headerReader.read_8();
                int ddam = headerReader.read_8();
                int status = headerReader.read_8();
                headerReader.skip(9); // skip ST0, ST1, ST2, PDA, reserved(5)
                if (cylinder == 0xFF)
                    continue;
                if (ddam != 0)
                    Error() << "NFD: nonzero ddam currently unsupported";
                if (status != 0)
                    Error() << "NFD: nonzero fdd status codes are currently "
                               "unsupported";
                if (currentTrackCylinder < 0)
                {
                    currentTrackCylinder = cylinder;
                    currentTrackHead = head;
                }
                else if (currentTrackCylinder != cylinder)
                {
                    Error() << "NFD: all sectors in a track must belong to the "
                               "same cylinder";
                }
                else if (currentTrackHead != head)
                {
                    Error() << "NFD: all sectors in a track must belong to the "
                               "same head";
                }
                if (trackSectorSize < 0)
                {
                    trackSectorSize = sectorSize;
                    // this is the first sector we've read, use it settings for
                    // per-track data
                    trackdata->set_cylinder(cylinder);
                    trackdata->set_head(head);
                    trackdata->set_sector_size(sectorSize);
                    trackdata->set_use_fm(!mfm);
                    if (!mfm)
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
                    Error() << "NFD: multiple sector sizes per track are "
                               "currently unsupported";
                }
                Bytes data(sectorSize);
                inputFile.read((char*)data.begin(), data.size());
                const auto& sector = image->put(cylinder, head, sectorId);
                sector->status = Sector::OK;
                sector->logicalTrack = cylinder;
                sector->physicalCylinder = cylinder;
                sector->logicalSide = sector->physicalHead = head;
                sector->logicalSector = sectorId;
                sector->data = data;

                sectors->add_sector(sectorId);
                if (config.cylinders().end() < cylinder)
                    config.mutable_cylinders()->set_end(cylinder);
            }
        }

        image->calculateSize();
        const Geometry& geometry = image->getGeometry();
        Logger() << fmt::format("NFD: read {} tracks, {} sides",
            geometry.numTracks,
            geometry.numSides);
        return image;
    }
};

std::unique_ptr<ImageReader> ImageReader::createNFDImageReader(
    const ImageReaderProto& config)
{
    return std::unique_ptr<ImageReader>(new NFDImageReader(config));
}
