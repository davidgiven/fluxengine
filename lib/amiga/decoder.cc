#include "globals.h"
#include "fluxmap.h"
#include "protocol.h"
#include "record.h"
#include "decoders.h"
#include "amiga.h"
#include <string.h>
#include <algorithm>

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
