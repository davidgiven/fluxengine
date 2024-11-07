#include "lib/core/globals.h"
#include "lib/data/fluxmap.h"
#include "lib/data/fluxmapreader.h"
#include "lib/data/fluxpattern.h"
#include "lib/decoders/decoders.h"
#include "lib/data/sector.h"
#include "micropolis.h"
#include "lib/core/bytes.h"
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

static uint8_t b(uint32_t field, uint8_t pos)
{
    return (field >> pos) & 1;
}

static uint8_t eccNextBit(uint32_t ecc, uint8_t data_bit)
{
    // This is 0x81932080 which is 0x0104C981 with reversed bits
    return b(ecc, 7) ^ b(ecc, 13) ^ b(ecc, 16) ^ b(ecc, 17) ^ b(ecc, 20) ^
           b(ecc, 23) ^ b(ecc, 24) ^ b(ecc, 31) ^ data_bit;
}

uint32_t vectorGraphicEcc(const Bytes& bytes)
{
    uint32_t e = 0;
    Bytes payloadBytes = bytes.slice(0, bytes.size() - 4);
    ByteReader payload(payloadBytes);
    while (!payload.eof())
    {
        uint8_t byte = payload.read_8();
        for (int i = 0; i < 8; i++)
        {
            e = (e << 1) | eccNextBit(e, byte >> 7);
            byte <<= 1;
        }
    }
    Bytes trailerBytes = bytes.slice(bytes.size() - 4);
    ByteReader trailer(trailerBytes);
    uint32_t res = e;
    while (!trailer.eof())
    {
        uint8_t byte = trailer.read_8();
        for (int i = 0; i < 8; i++)
        {
            res = (res << 1) | eccNextBit(e, byte >> 7);
            e <<= 1;
            byte <<= 1;
        }
    }
    return res;
}

/* Fixes bytes when possible, returning true if changed. */
static bool vectorGraphicEccFix(Bytes& bytes, uint32_t syndrome)
{
    uint32_t ecc = syndrome;
    int pos = (MICROPOLIS_ENCODED_SECTOR_SIZE - 5) * 8 + 7;
    bool aligned = false;
    while ((ecc & 0xff000000) == 0)
    {
        pos += 8;
        ecc <<= 8;
    }
    for (; pos >= 0; pos--)
    {
        bool bit = ecc & 1;
        ecc >>= 1;
        if (bit)
            ecc ^= 0x808264c0;
        if ((ecc & 0xff07ffff) == 0)
            aligned = true;
        if (aligned && pos % 8 == 0)
            break;
    }
    if (pos < 0)
        return false;
    bytes[pos / 8] ^= ecc >> 16;
    return true;
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

        bool eccPresent = bytes[274] == 0xaa;
        uint32_t ecc = 0;
        if (_config.ecc_type() == MicropolisDecoderProto::VECTOR && eccPresent)
        {
            ecc = vectorGraphicEcc(bytes.slice(0, 274));
            if (ecc != 0)
            {
                vectorGraphicEccFix(bytes, ecc);
                ecc = vectorGraphicEcc(bytes.slice(0, 274));
            }
        }

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
        if (wantChecksum == gotChecksum && (!eccPresent || ecc == 0))
            _sector->status = Sector::OK;
        else
            _sector->status = Sector::BAD_CHECKSUM;
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
