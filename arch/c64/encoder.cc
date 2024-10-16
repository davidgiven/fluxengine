#include "lib/core/globals.h"
#include "lib/core/utils.h"
#include "lib/decoders/decoders.h"
#include "lib/encoders/encoders.h"
#include "c64.h"
#include "lib/core/crc.h"
#include "lib/data/sector.h"
#include "lib/data/image.h"
#include "fmt/format.h"
#include "arch/c64/c64.pb.h"
#include "lib/encoders/encoders.pb.h"
#include "lib/data/layout.h"
#include <ctype.h>
#include "lib/core/bytes.h"

static bool lastBit;

static int encode_data_gcr(uint8_t data)
{
    switch (data)
    {
#define GCR_ENTRY(gcr, data) \
    case data:               \
        return gcr;
#include "data_gcr.h"
#undef GCR_ENTRY
    }
    return -1;
}

static void write_bits(
    std::vector<bool>& bits, unsigned& cursor, const std::vector<bool>& src)
{
    for (bool bit : src) // Range-based for loop
    {
        if (cursor < bits.size())
            bits[cursor++] = bit;
    }
}

static void write_bits(
    std::vector<bool>& bits, unsigned& cursor, uint64_t data, int width)
{
    cursor += width;
    for (int i = 0; i < width; i++)
    {
        unsigned pos = cursor - i - 1;
        if (pos < bits.size())
            bits[pos] = data & 1;
        data >>= 1;
    }
}

static std::vector<bool> encode_data(uint8_t input)
{
    /*
     * Four 8-bit data bytes are converted to four 10-bit GCR bytes at a time by
     * the 1541 DOS.  RAM is only an 8-bit storage device though. This hardware
     * limitation prevents a 10-bit GCR byte from being stored in a single
     * memory location. Four 10-bit GCR bytes total 40 bits - a number evenly
     * divisible by our overriding 8-bit constraint. Commodore sub- divides the
     * 40 GCR bits into five 8-bit bytes to solve this dilemma. This explains
     * why four 8-bit data bytes are converted to GCR form at a time. The
     * following step by step example demonstrates how this bit manipulation is
     * performed by the DOS.
     *
     * STEP 1. Four 8-bit Data Bytes
     * $08 $10 $00 $12
     *
     * STEP 2. Hexadecimal to Binary Conversion
     * 1. Binary Equivalents
     * $08       $10         $00         $12
     * 00001000  00010000    00000000    00010010
     *
     * STEP 3. Binary to GCR Conversion
     * 1. Four 8-bit Data Bytes
     * 00001000 00010000 00000000 00010010
     * 2. High and Low Nybbles
     * 0000 1000 0001 0000 0000 0000 0001 0010
     * 3. High and Low Nybble GCR Equivalents
     * 01010 01001 01011 01010 01010 01010 01011 10010
     * 4. Four 10-bit GCR Bytes
     * 0101001001 0101101010 0101001010 0101110010
     *
     * STEP 4. 10-bit GCR to 8-bit GCR Conversion
     *   1. Concatenate Four 10-bit GCR Bytes
     *   0101001001010110101001010010100101110010
     *   2. Five 8-bit Subdivisions
     *   01010010 01010110 10100101 00101001 01110010
     *
     * STEP 5. Binary to Hexadecimal Conversion
     * 1. Hexadecimal Equivalents
     *   01010010    01010110    10100101    00101001    01110010
     *   $52         $56         $A5         $29         $72
     *
     * STEP 6. Four 8-bit Data Bytes are Recorded as Five 8-bit GCR Bytes
     *   $08 $10 $00 $12
     *
     * are recorded as
     *   $52 $56 $A5 $29 $72
     */

    std::vector<bool> output(10, false);
    uint8_t hi = 0;
    uint8_t lo = 0;
    uint8_t lo_GCR = 0;
    uint8_t hi_GCR = 0;

    // Convert the byte in high and low nibble
    lo = input >> 4; // get the lo nibble shift the bits 4 to the right
    hi = input & 15; // get the hi nibble bij masking the lo bits (00001111)

    lo_GCR = encode_data_gcr(lo); // example value: 0000   GCR = 01010
    hi_GCR = encode_data_gcr(hi); // example value: 1000   GCR = 01001
    // output = [0,1,2,3,4,5,6,7,8,9]
    // value  = [0,1,0,1,0,0,1,0,0,1]
    //           01010 01001

    int b = 4;
    for (int i = 0; i < 10; i++)
    {
        if (i < 5)                        // 01234
        {                                 // i = 0 op
            output[4 - i] = (lo_GCR & 1); // 01010

            // 01010 -> & 00001 -> 00000 output[4] = 0
            // 00101 -> & 00001 -> 00001 output[3] = 1
            // 00010 -> & 00001 -> 00000 output[2] = 0
            // 00001 -> & 00001 -> 00001 output[1] = 1
            // 00000 -> & 00001 -> 00000 output[0] = 0
            lo_GCR >>= 1;
        }
        else
        {
            output[i + b] = (hi_GCR & 1); // 01001
            // 01001 -> & 00001 -> 00001 output[9] = 1
            // 00100 -> & 00001 -> 00000 output[8] = 0
            // 00010 -> & 00001 -> 00000 output[7] = 0
            // 00001 -> & 00001 -> 00001 output[6] = 1
            // 00000 -> & 00001 -> 00000 output[5] = 0
            hi_GCR >>= 1;
            b = b - 2;
        }
    }
    return output;
}

class Commodore64Encoder : public Encoder
{
public:
    Commodore64Encoder(const EncoderProto& config):
        Encoder(config),
        _config(config.c64())
    {
    }

public:
    std::unique_ptr<Fluxmap> encode(std::shared_ptr<const TrackInfo>& trackInfo,
        const std::vector<std::shared_ptr<const Sector>>& sectors,
        const Image& image) override
    {
        /* The format ID Character # 1 and # 2 are in the .d64 image only
         * present in track 18 sector zero which contains the BAM info in byte
         * 162 and 163. it is written in every header of every sector and track.
         * headers are not stored in a d64 disk image so we have to get it from
         * track 18 which contains the BAM.
         */

        const auto& sectorData = image.get(
            C64_BAM_TRACK, 0, 0); // Read de BAM to get the DISK ID bytes
        if (sectorData)
        {
            ByteReader br(sectorData->data);
            br.seek(162); // goto position of the first Disk ID Byte
            _formatByte1 = br.read_8();
            _formatByte2 = br.read_8();
        }
        else
            _formatByte1 = _formatByte2 = 0;

        double clockRateUs = clockPeriodForC64Track(trackInfo->logicalTrack);
        int bitsPerRevolution = 200000.0 / clockRateUs;

        std::vector<bool> bits(bitsPerRevolution);
        unsigned cursor = 0;

        fillBitmapTo(bits,
            cursor,
            _config.post_index_gap_us() / clockRateUs,
            {true, false});
        lastBit = false;

        for (const auto& sector : sectors)
            writeSector(bits, cursor, sector);

        if (cursor >= bits.size())
            error("track data overrun by {} bits", cursor - bits.size());
        fillBitmapTo(bits, cursor, bits.size(), {true, false});

        std::unique_ptr<Fluxmap> fluxmap(new Fluxmap);
        fluxmap->appendBits(
            bits, calculatePhysicalClockPeriod(clockRateUs * 1e3, 200e6));
        return fluxmap;
    }

private:
    void writeSector(std::vector<bool>& bits,
        unsigned& cursor,
        std::shared_ptr<const Sector> sector) const
    {
        /* Source: http://www.unusedino.de/ec64/technical/formats/g64.html
         * 1. Header sync       FF FF FF FF FF (40 'on' bits, not GCR)
         * 2. Header info       52 54 B5 29 4B 7A 5E 95 55 55 (10 GCR bytes)
         * 3. Header gap        55 55 55 55 55 55 55 55 55 (9 bytes, never read)
         * 4. Data sync         FF FF FF FF FF (40 'on' bits, not GCR)
         * 5. Data block        55...4A (325 GCR bytes)
         * 6. Inter-sector gap  55 55 55 55...55 55 (4 to 12 bytes, never read)
         * 1. Header sync       (SYNC for the next sector)
         */
        if ((sector->status == Sector::OK) or
            (sector->status == Sector::BAD_CHECKSUM))
        {
            // There is data to encode to disk.
            if ((sector->data.size() != C64_SECTOR_LENGTH))
                error("unsupported sector size {} --- you must pick 256",
                    sector->data.size());

            // 1. Write header Sync (not GCR)
            for (int i = 0; i < 6; i++)
                write_bits(
                    bits, cursor, C64_HEADER_DATA_SYNC, 1 * 8); /* sync */

            // 2. Write Header info 10 GCR bytes
            /*
             * The 10 byte header info (#2) is GCR encoded and must be decoded
             * to it's normal 8 bytes to be understood. Once decoded, its
             * breakdown is as follows:
             *
             * Byte $00 - header block ID           ($08)
             *   01 - header block checksum 16  (EOR of $02-$05)
             *   02 - Sector
             *   03 - Track
             *   04 - Format ID byte #2
             *   05 - Format ID byte #1
             *   06-07 - $0F ("off" bytes)
             */
            uint8_t encodedTrack =
                ((sector->logicalTrack) +
                    1); // C64 track numbering starts with 1. Fluxengine with 0.
            uint8_t encodedSector = sector->logicalSector;
            // uint8_t formatByte1 = C64_FORMAT_ID_BYTE1;
            // uint8_t formatByte2 = C64_FORMAT_ID_BYTE2;
            uint8_t headerChecksum =
                (encodedTrack ^ encodedSector ^ _formatByte1 ^ _formatByte2);
            write_bits(bits, cursor, encode_data(C64_HEADER_BLOCK_ID));
            write_bits(bits, cursor, encode_data(headerChecksum));
            write_bits(bits, cursor, encode_data(encodedSector));
            write_bits(bits, cursor, encode_data(encodedTrack));
            write_bits(bits, cursor, encode_data(_formatByte2));
            write_bits(bits, cursor, encode_data(_formatByte1));
            write_bits(bits, cursor, encode_data(C64_PADDING));
            write_bits(bits, cursor, encode_data(C64_PADDING));

            // 3. Write header GAP not GCR
            for (int i = 0; i < 9; i++)
                write_bits(
                    bits, cursor, C64_HEADER_GAP, 1 * 8); /* header gap */

            // 4. Write Data sync not GCR
            for (int i = 0; i < 6; i++)
                write_bits(
                    bits, cursor, C64_HEADER_DATA_SYNC, 1 * 8); /* sync */

            // 5. Write data block 325 GCR bytes
            /*
             * The 325 byte data block (#5) is GCR encoded and must be  decoded
             * to  its normal 260 bytes to be understood. The data block is made
             * up of the following:
             *
             * Byte    $00 - data block ID ($07)
             *      01-100 - 256 bytes data
             *      101 - data block checksum (EOR of $01-100)
             *      102-103 - $00 ("off" bytes, to make the sector size a
             * multiple of 5)
             */

            write_bits(bits, cursor, encode_data(C64_DATA_BLOCK_ID));
            uint8_t dataChecksum = xorBytes(sector->data);
            ByteReader br(sector->data);
            int i = 0;
            for (i = 0; i < C64_SECTOR_LENGTH; i++)
            {
                uint8_t val = br.read_8();
                write_bits(bits, cursor, encode_data(val));
            }
            write_bits(bits, cursor, encode_data(dataChecksum));
            write_bits(bits, cursor, encode_data(C64_PADDING));
            write_bits(bits, cursor, encode_data(C64_PADDING));

            // 6. Write inter-sector gap 9 - 12 bytes nor gcr
            for (int i = 0; i < 9; i++)
                write_bits(
                    bits, cursor, C64_INTER_SECTOR_GAP, 1 * 8); /* sync */
        }
    }

private:
    const Commodore64EncoderProto& _config;
    uint8_t _formatByte1;
    uint8_t _formatByte2;
};

std::unique_ptr<Encoder> createCommodore64Encoder(const EncoderProto& config)
{
    return std::unique_ptr<Encoder>(new Commodore64Encoder(config));
}

// vim: sw=4 ts=4 et
