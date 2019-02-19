#include "globals.h"
#include "fluxmap.h"
#include "protocol.h"
#include "record.h"
#include "decoders.h"
#include "sector.h"
#include "amiga.h"
#include "bytes.h"
#include "fmt/format.h"
#include <string.h>
#include <algorithm>

/* 
 * Amiga disks use MFM but it's not quite the same as IBM MFM. They only use
 * a single type of record with a different marker byte.
 * 
 * See the big comment in the IBM MFM decoder for the gruesome details of how
 * MFM works.
 */
         
static std::vector<uint8_t> deinterleave(std::vector<uint8_t>::const_iterator& input, size_t len)
{
    assert(!(len & 1));
    const uint8_t* odds = &input[0];
    const uint8_t* evens = &input[len/2];
    std::vector<uint8_t> output;
    output.reserve(len);

    for (size_t i=0; i<len/2; i++)
    {
        uint8_t o = *odds++;
        uint8_t e = *evens++;

        /* This is the 'Interleave bits with 64-bit multiply' technique from
         * http://graphics.stanford.edu/~seander/bithacks.html#InterleaveBMN
         */
        uint16_t result =
            (((e * 0x0101010101010101ULL & 0x8040201008040201ULL)
                * 0x0102040810204081ULL >> 49) & 0x5555) |
            (((o * 0x0101010101010101ULL & 0x8040201008040201ULL)
                * 0x0102040810204081ULL >> 48) & 0xAAAA);
        
        output.push_back((uint8_t)(result >> 8));
        output.push_back((uint8_t)result);
    }

    input += len;
    return output;
}

static uint32_t checksum(std::vector<uint8_t>::const_iterator input, size_t len)
{
    uint32_t checksum = 0;

    assert((len & 3) == 0);
    while (len != 0)
    {
        checksum ^= read_be32(input);
        input += 4;
        len -= 4;
    }

    return checksum & 0x55555555;
}

SectorVector AmigaDecoder::decodeToSectors(const RawRecordVector& rawRecords)
{
    std::vector<std::unique_ptr<Sector>> sectors;

    for (auto& rawrecord : rawRecords)
    {
        const auto& rawdata = rawrecord->data;
        const auto& rawbytes = toBytes(rawdata);
        const auto& bytes = decodeFmMfm(rawdata);

        if (bytes.size() < 544)
            continue;

        auto ptr = bytes.begin() + 4;

        std::vector<uint8_t> header = deinterleave(ptr, 4);
        std::vector<uint8_t> recoveryinfo = deinterleave(ptr, 16);

        uint32_t wantedheaderchecksum = read_be32(deinterleave(ptr, 4));
        uint32_t gotheaderchecksum = checksum(rawbytes.begin() + 8, 40);
        if (gotheaderchecksum != wantedheaderchecksum)
            continue;

        uint32_t wanteddatachecksum = read_be32(deinterleave(ptr, 4));
        uint32_t gotdatachecksum = checksum(rawbytes.begin() + 64, 1024);
        int status = (gotdatachecksum == wanteddatachecksum) ? Sector::OK : Sector::BAD_CHECKSUM;

        std::vector<uint8_t> databytes = deinterleave(ptr, 512);
        unsigned track = header[1] >> 1;
        unsigned side = header[1] & 1;
        auto sector = std::unique_ptr<Sector>(
            new Sector(status, track, side, header[2], databytes));
        sectors.push_back(std::move(sector));
	}

	return sectors;
}

nanoseconds_t AmigaDecoder::guessClock(Fluxmap& fluxmap) const
{
    return fluxmap.guessClock() / 2;
}

int AmigaDecoder::recordMatcher(uint64_t fifo) const
{
    uint64_t masked = fifo & 0xffffffffffffULL;
    if (masked == AMIGA_SECTOR_RECORD)
		return 64;
    return 0;
}

#if 0
static size_t cursor;
static size_t recordstart;
static std::vector<uint8_t> outputbuffer;
static uint8_t outputfifo = 0;
static int bitcount = 0;
static bool phase = false;

static void write_bit(bool bit)
{
    outputfifo = (outputfifo << 1) | bit;
    bitcount++;
    if (bitcount == 8)
    {
        outputbuffer.push_back(outputfifo);
        bitcount = 0;
    }
}

static void add_record(RecordVector& records,
	nanoseconds_t position, const std::vector<uint8_t>& data)
{
	records.push_back(std::unique_ptr<Record>(new Record(position, data)));
}

nanoseconds_t AmigaBitmapDecoder::guessClock(Fluxmap& fluxmap) const
{
    return fluxmap.guessClock()/2;
}

RecordVector AmigaBitmapDecoder::decodeBitsToRecords(const std::vector<bool>& bits) const
{
    RecordVector records;

    cursor = 0;
    uint64_t inputfifo = 0;
    bool reading = false;

    while (cursor < bits.size())
    {
        bool bit = bits[cursor++];
        inputfifo = (inputfifo << 1) | bit;

        /* 
         * Amiga disks use MFM but it's not quite the same as IBM MFM. They only use
         * a single type of record with a different marker byte.
         * 
         * See the big comment in the IBM MFM decoder for the gruesome details of how
         * MFM works.
         */
         
        if (inputfifo == 0xaaaaaaaa44894489)
        {
            if (reading)
				add_record(records, recordstart, outputbuffer);
            recordstart = cursor - 4*4*8;

            outputbuffer.resize(4);
            outputbuffer[0] = outputbuffer[1] = 0x00;
            outputbuffer[2] = outputbuffer[3] = 0xa1;

            reading = true;
            bitcount = 0;
            phase = 0;
        }
        else if (reading)
        {
            if (phase)
                write_bit(bit);
            phase = !phase;
        }
    }

    if (reading)
		add_record(records, recordstart, outputbuffer);

    return records;
}
#endif
