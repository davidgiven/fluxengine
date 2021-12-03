#include "globals.h"
#include "flags.h"
#include "sector.h"
#include "imagereader/imagereader.h"
#include "image.h"
#include "lib/config.pb.h"
#include "imagereader/imagereaderimpl.h"
#include "fmt/format.h"
#include <algorithm>
#include <iostream>
#include <fstream>

// reader based on this partial documentation of the D88 format:
// https://www.pc98.org/project/doc/d88.html

class D88ImageReader : public ImageReader
{
public:
	D88ImageReader(const ImageReaderProto& config):
		ImageReader(config)
	{}

	std::unique_ptr<Image> readImage()
	{
        std::ifstream inputFile(_config.filename(), std::ios::in | std::ios::binary);
        if (!inputFile.is_open())
            Error() << "cannot open input file";

        Bytes header(0x24); // read first entry of track table as well
        inputFile.read((char*) header.begin(), header.size());

        // the DIM header technically has a bit field for sectors present,
        // however it is currently ignored by this reader

        std::string diskName = header.slice(0, 0x16);

        if (diskName[0])
            std::cout << "D88: disk name: " << diskName << "\n";

        ByteReader headerReader(header);

        // media flag indicates media density, currently unused
        char mediaFlag = headerReader.seek(0x1b).read_8();

        inputFile.seekg( 0, std::ios::end );
        int fileSize = inputFile.tellg();

        int diskSize = headerReader.seek(0x1c).read_le32();

        if (diskSize > fileSize)
            std::cout << "D88: found multiple disk images. Only using first\n";

        int trackTableEnd = headerReader.seek(0x20).read_le32();
        int trackTableSize = trackTableEnd - 0x20;

        Bytes trackTable(trackTableSize);
        inputFile.seekg(0x20);
        inputFile.read((char*) trackTable.begin(), trackTable.size());
        ByteReader trackTableReader(trackTable);

        int diskSectorsPerTrack = -1;
        int diskSectorSize = -1;

        std::unique_ptr<Image> image(new Image);
        for (int track = 0; track < trackTableSize / 4; track++)
        {
            int trackOffset = trackTableReader.seek(track * 4).read_le32();
            if (trackOffset == 0) continue;

            int currentTrackOffset = trackOffset;
            int currentTrackCylinder = -1;
            int currentSectorsInTrack = 0xffff; // don't know # of sectors until we read the first one
            for (int sectorInTrack = 0; sectorInTrack < currentSectorsInTrack; sectorInTrack++){
                Bytes sectorHeader(0x10);
                inputFile.read((char*) sectorHeader.begin(), sectorHeader.size());
                ByteReader sectorHeaderReader(sectorHeader);
                char cylinder = sectorHeaderReader.seek(0).read_8();
                char head = sectorHeaderReader.seek(1).read_8();
                char sectorId = sectorHeaderReader.seek(2).read_8();
                int sectorSize = 128 << sectorHeaderReader.seek(3).read_8();
                int sectorsInTrack = sectorHeaderReader.seek(4).read_le16();
                int mfm = sectorHeaderReader.seek(6).read_8();
                int ddam = sectorHeaderReader.seek(7).read_8();
                int fddStatusCode = sectorHeaderReader.seek(8).read_8();
                // D88 provides much more sector information that is currently ignored
                if (ddam != 0)
                    Error() << "D88: nonzero ddam currently unsupported";
                if (fddStatusCode != 0)
                    Error() << "D88: nonzero fdd status codes are currently unsupported";
                if (currentSectorsInTrack == 0xffff) {
                    currentSectorsInTrack = sectorsInTrack;
                } else if (currentSectorsInTrack != sectorsInTrack) {
                    Error() << "D88: mismatched number of sectors in track";
                }
                if (diskSectorsPerTrack < 0) {
                    diskSectorsPerTrack = sectorsInTrack;
                } else if (diskSectorsPerTrack != sectorsInTrack) {
                    Error() << "D88: varying numbers of sectors per track is currently unsupported";
                }
                if (diskSectorSize < 0) {
                    diskSectorSize = sectorSize;
                } else if (diskSectorSize != sectorSize) {
                    Error() << "D88: variable sector sizes are currently unsupported";
                }
                if (mfm != 0)
                    Error() << "D88: Non-MFM sectors are currenty unsupported";
                if (currentTrackCylinder < 0) {
                    currentTrackCylinder = cylinder;
                } else if (currentTrackCylinder != cylinder) {
                    Error() << "D88: all sectors in a track must belong to the same cylinder";
                }
                Bytes data(sectorSize);
                inputFile.read((char*) data.begin(), data.size());
                const auto& sector = image->put(cylinder, head, sectorId);
                sector->status = Sector::OK;
                sector->logicalTrack = cylinder;
                sector->physicalCylinder = cylinder;
                sector->logicalSide = sector->physicalHead = head;
                sector->logicalSector = sectorId;
                sector->data = data;
            }
        }

        image->calculateSize();
        const Geometry& geometry = image->getGeometry();
        std::cout << fmt::format("D88: read {} tracks, {} sides\n",
                        geometry.numTracks, geometry.numSides);
        return image;
    }

};

std::unique_ptr<ImageReader> ImageReader::createD88ImageReader(
	const ImageReaderProto& config)
{
    return std::unique_ptr<ImageReader>(new D88ImageReader(config));
}

