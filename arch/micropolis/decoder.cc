#include "globals.h"
#include "fluxmap.h"
#include "decoders/fluxmapreader.h"
#include "decoders/decoders.h"
#include "sector.h"
#include "micropolis.h"
#include "bytes.h"
#include "fmt/format.h"
#include "lib/decoders/decoders.pb.h"

/* The sector has a preamble of MFM 0x00s and uses 0xFF as a sync pattern.
 *
 * 00        00        00        F         F
 * 0000 0000 0000 0000 0000 0000 0101 0101 0101 0101
 * A    A    A    A    A    A    5    5    5    5
 */
static const FluxPattern SECTOR_SYNC_PATTERN(64, 0xAAAAAAAAAAAA5555LL);

/* Pattern to skip past current SYNC. */
static const FluxPattern SECTOR_ADVANCE_PATTERN(64, 0xAAAAAAAAAAAAAAAALL);

/* Standard Micropolis checksum.  Adds all bytes, with carry. */
uint8_t micropolisChecksum(const Bytes& bytes)
{
    ByteReader br(bytes);
    uint16_t sum = 0;
    while (!br.eof())
    {
        if (sum > 0xFF)
        {
            sum -= 0x100 - 1;
        }
        sum += br.read_8();
    }
    /* The last carry is ignored */
    return sum & 0xFF;
}

/* Vector MZOS does not use the standard Micropolis checksum.
 * The checksum is initially 0.
 * For each data byte in the 256-byte payload, rotate left,
 * carrying bit 7 to bit 0.  XOR with the current checksum.
 *
 * Unlike the Micropolis checksum, this does not cover the 12-byte
 * header (track, sector, 10 OS-specific bytes.)
 */
uint8_t mzosChecksum(const Bytes& bytes)
{
    ByteReader br(bytes);
    uint8_t checksum = 0;
    uint8_t databyte;

    while (!br.eof())
    {
        databyte = br.read_8();
        checksum ^= ((databyte << 1) | (databyte >> 7));
    }

    return checksum;
}

class MicropolisDecoder : public Decoder
{
public:
    MicropolisDecoder(const DecoderProto& config):
        Decoder(config),
        _config(config.micropolis())
    {
        _checksumType = _config.checksum_type();
    }

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
         * rare, but has been observed on some disks. There's 570uS of slack in
         * each sector, after accounting for preamble, data, and postamble.
         */
        if (now > (getFluxmapDuration() - 12.0e6))
        {
            seekToIndexMark();
            return 0;
        }

        nanoseconds_t clock = seekToPattern(SECTOR_SYNC_PATTERN);

        auto syncDelta = tell().ns() - now;
        /* Due to the weak nature of the Micropolis SYNC patern,
         * it's possible to detect a false SYNC during the gap
         * between the sector pulse and the write gate.  If the SYNC
         * is detected less than 100uS after the sector pulse, search
         * for another valid SYNC.
         *
         * Reference: Vector Micropolis Disk Controller Board Technical
         * Information Manual, pp. 1-16.
         */
        if ((syncDelta > 0) && (syncDelta < 100e3))
        {
            seekToPattern(SECTOR_ADVANCE_PATTERN);
            clock = seekToPattern(SECTOR_SYNC_PATTERN);
        }

        _sector->headerStartTime = tell().ns();

        /* seekToPattern() can skip past the index hole, if this happens
         * too close to the end of the Fluxmap, discard the sector. The
         * preamble was expected to be 640uS long.
         */
        if (_sector->headerStartTime > (getFluxmapDuration() - 11.3e6))
        {
            return 0;
        }

        return clock;
    }

    void decodeSectorRecord() override
    {
        readRawBits(48);
        auto rawbits = readRawBits(MICROPOLIS_ENCODED_SECTOR_SIZE * 16);
        auto bytes =
            decodeFmMfm(rawbits).slice(0, MICROPOLIS_ENCODED_SECTOR_SIZE);
        ByteReader br(bytes);

        int syncByte = br.read_8(); /* sync */
        if (syncByte != 0xFF)
            return;

        _sector->logicalTrack = br.read_8();
        _sector->logicalSide = _sector->physicalSide;
        _sector->logicalSector = br.read_8();
        if (_sector->logicalSector > 15)
            return;
        if (_sector->logicalTrack > 76)
            return;
        if (_sector->logicalTrack != _sector->physicalTrack)
            return;

        br.read(10); /* OS data or padding */
        auto data = br.read(MICROPOLIS_PAYLOAD_SIZE);
        uint8_t wantChecksum = br.read_8();

        /* If not specified, automatically determine the checksum type.
         * Once the checksum type is determined, it will be used for the
         * entire disk.
         */
        if (_checksumType == MicropolisDecoderProto::AUTO)
        {
            /* Calculate both standard Micropolis (MDOS, CP/M, OASIS) and MZOS
             * checksums */
            if (wantChecksum == micropolisChecksum(bytes.slice(1, 2 + 266)))
            {
                _checksumType = MicropolisDecoderProto::MICROPOLIS;
            }
            else if (wantChecksum ==
                     mzosChecksum(bytes.slice(
                         MICROPOLIS_HEADER_SIZE, MICROPOLIS_PAYLOAD_SIZE)))
            {
                _checksumType = MicropolisDecoderProto::MZOS;
                std::cout << "Note: MZOS checksum detected." << std::endl;
            }
        }

        uint8_t gotChecksum;

        if (_checksumType == MicropolisDecoderProto::MZOS)
        {
            gotChecksum = mzosChecksum(
                bytes.slice(MICROPOLIS_HEADER_SIZE, MICROPOLIS_PAYLOAD_SIZE));
        }
        else
        {
            gotChecksum = micropolisChecksum(bytes.slice(1, 2 + 266));
        }

        br.read(5); /* 4 byte ECC and ECC-present flag */

        if (_config.sector_output_size() == MICROPOLIS_PAYLOAD_SIZE)
            _sector->data = data;
        else if (_config.sector_output_size() == MICROPOLIS_ENCODED_SECTOR_SIZE)
            _sector->data = bytes;
        else
            error("Sector output size may only be 256 or 275");
        _sector->status =
            (wantChecksum == gotChecksum) ? Sector::OK : Sector::BAD_CHECKSUM;
    }

private:
    const MicropolisDecoderProto& _config;
    MicropolisDecoderProto_ChecksumType
        _checksumType; /* -1 = auto, 1 = Micropolis, 2=MZOS */
};

std::unique_ptr<Decoder> createMicropolisDecoder(const DecoderProto& config)
{
    return std::unique_ptr<Decoder>(new MicropolisDecoder(config));
}
