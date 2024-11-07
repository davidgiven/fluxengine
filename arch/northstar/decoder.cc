/* Decoder for North Star 10-sector hard-sectored disks.
 *
 * Supports both single- and double-density.  For the sector format and
 * checksum algorithm, see pp. 33 of the North Star Double Density Controller
 * manual:
 *
 * http://bitsavers.org/pdf/northstar/boards/Northstar_MDS-A-D_1978.pdf
 *
 * North Star disks do not contain any track/head/sector information
 * encoded in the sector record.  For this reason, we have to be absolutely
 * sure that the hardSectorId is correct.
 */

#include "lib/core/globals.h"
#include "lib/data/fluxmap.h"
#include "lib/data/fluxmapreader.h"
#include "lib/data/fluxpattern.h"
#include "lib/decoders/decoders.h"
#include "lib/data/sector.h"
#include "northstar.h"
#include "lib/core/bytes.h"
#include "lib/decoders/decoders.pb.h"
#include "fmt/format.h"

#define MFM_ID 0xaaaaaaaaaaaa5545LL
#define FM_ID 0xaaaaaaaaaaaaffefLL
/*
 * MFM sectors have 32 bytes of 00's followed by two sync characters,
 * specified in the North Star MDS manual as 0xFBFB.
 *
 * This is true for most disks; however, I found a few disks, including an
 * original North Star DOS/BASIC v2.2.1 DQ disk) that uses 0xFBnn, where
 * nn is an incrementing pattern.
 *
 * 00        00        00        F         B
 * 0000 0000 0000 0000 0000 0000 0101 0101 0100 0101
 * A    A    A    A    A    A    5    5    4    5
 */
static const FluxPattern MFM_PATTERN(64, MFM_ID);

/* FM sectors have 16 bytes of 00's followed by 0xFB.
 * 00        FB
 * 0000 0000 1111 1111 1110 1111
 * A    A    F    F    E    F
 */
static const FluxPattern FM_PATTERN(64, FM_ID);

const FluxMatchers ANY_SECTOR_PATTERN({
    &MFM_PATTERN,
    &FM_PATTERN,
});

/* Checksum is initially 0.
 * For each data byte, XOR with the current checksum.
 * Rotate checksum left, carrying bit 7 to bit 0.
 */
uint8_t northstarChecksum(const Bytes& bytes)
{
    ByteReader br(bytes);
    uint8_t checksum = 0;

    while (!br.eof())
    {
        checksum ^= br.read_8();
        checksum = ((checksum << 1) | ((checksum >> 7)));
    }

    return checksum;
}

class NorthstarDecoder : public Decoder
{
public:
    NorthstarDecoder(const DecoderProto& config):
        Decoder(config),
        _config(config.northstar())
    {
    }

    /* Search for FM or MFM sector record */
    nanoseconds_t advanceToNextRecord() override
    {
        nanoseconds_t now = tell().ns();

        /* For all but the first sector, seek to the next sector pulse.
         * The first sector does not contain the sector pulse in the fluxmap.
         */
        if (now != 0)
        {
            seekToIndexMark();
            now = tell().ns();
        }

        /* Discard a possible partial sector at the end of the track.
         * This partial sector could be mistaken for a conflicted sector, if
         * whatever data read happens to match the checksum of 0, which is
         * rare, but has been observed on some disks.
         */
        if (now > (getFluxmapDuration() - 21e6))
        {
            seekToIndexMark();
            return 0;
        }

        int msSinceIndex = std::round(now / 1e6);

        /* Note that the seekToPattern ignores the sector pulses, so if
         * a sector is not found for some reason, the seek will advance
         * past one or more sector pulses.  For this reason, calculate
         * _hardSectorId after the sector header is found.
         */
        nanoseconds_t clock = seekToPattern(ANY_SECTOR_PATTERN);
        _sector->headerStartTime = tell().ns();

        /* Discard a possible partial sector. */
        if (_sector->headerStartTime > (getFluxmapDuration() - 21e6))
        {
            return 0;
        }

        int sectorFoundTimeRaw = std::round(_sector->headerStartTime / 1e6);
        int sectorFoundTime;

        /* Round time to the nearest 20ms */
        if ((sectorFoundTimeRaw % 20) < 10)
        {
            sectorFoundTime = (sectorFoundTimeRaw / 20) * 20;
        }
        else
        {
            sectorFoundTime = ((sectorFoundTimeRaw + 20) / 20) * 20;
        }

        /* Calculate the sector ID based on time since the index */
        _hardSectorId = (sectorFoundTime / 20) % 10;

        return clock;
    }

    void decodeSectorRecord() override
    {
        uint64_t id = toBytes(readRawBits(64)).reader().read_be64();
        unsigned recordSize, payloadSize, headerSize;

        if (id == MFM_ID)
        {
            recordSize = NORTHSTAR_ENCODED_SECTOR_SIZE_DD;
            payloadSize = NORTHSTAR_PAYLOAD_SIZE_DD;
            headerSize = NORTHSTAR_HEADER_SIZE_DD;
        }
        else
        {
            recordSize = NORTHSTAR_ENCODED_SECTOR_SIZE_SD;
            payloadSize = NORTHSTAR_PAYLOAD_SIZE_SD;
            headerSize = NORTHSTAR_HEADER_SIZE_SD;
        }

        auto rawbits = readRawBits(recordSize * 16);
        auto bytes = decodeFmMfm(rawbits).slice(0, recordSize);
        ByteReader br(bytes);

        _sector->logicalSide = _sector->physicalSide;
        _sector->logicalSector = _hardSectorId;
        _sector->logicalTrack = _sector->physicalTrack;

        if (headerSize == NORTHSTAR_HEADER_SIZE_DD)
        {
            br.read_8(); /* MFM second Sync char, usually 0xFB */
        }

        _sector->data = br.read(payloadSize);
        uint8_t wantChecksum = br.read_8();
        uint8_t gotChecksum =
            northstarChecksum(bytes.slice(headerSize - 1, payloadSize));
        _sector->status =
            (wantChecksum == gotChecksum) ? Sector::OK : Sector::BAD_CHECKSUM;
    }

private:
    const NorthstarDecoderProto& _config;
    uint8_t _hardSectorId;
};

std::unique_ptr<Decoder> createNorthstarDecoder(const DecoderProto& config)
{
    return std::unique_ptr<Decoder>(new NorthstarDecoder(config));
}
