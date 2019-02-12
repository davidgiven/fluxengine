#include "globals.h"
#include "fluxmap.h"
#include "protocol.h"
#include "record.h"
#include "decoders.h"
#include <string.h>
#include <algorithm>

static size_t cursor;
static size_t recordstart;
static std::vector<uint8_t> outputbuffer;
static uint8_t outputfifo = 0;
static int bitcount = 0;
static bool phase = false;

static uint8_t extract_data_bits(uint16_t x)
{
    x = x & 0x5555;
    x = (x | (x >> 1)) & 0x3333;
    x = (x | (x >> 2)) & 0x0f0f;
    x = (x | (x >> 4)) & 0x00ff;
    return x;
}

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

nanoseconds_t FmBitmapDecoder::guessClock(Fluxmap& fluxmap) const
{
    return fluxmap.guessClock();
}

RecordVector FmBitmapDecoder::decodeBitsToRecords(const std::vector<bool>& bits) const
{
    RecordVector records;

    cursor = 0;
    uint16_t inputfifo = 0;
    bool reading = false;

    while (cursor < bits.size())
    {
        bool bit = bits[cursor++];
        inputfifo = (inputfifo << 1) | bit;

        /* 
         * FM is dumb as rocks, consisting on regular clock pulses with data pulses in
         * the gaps. Zero is:
         * 
         *     X-X-X-X-X-X-X-X-
         * 
         * However, the markers at the beginning of records are special, and have
         * missing clock pulses, allowing them to be found by the logic.
         * 
         * IAM record:
         * flux:   XXXX-XXX-XXXX-X- = 0xf77a
         * clock:  X X - X - X X X  = 0xd7
         * data:    X X X X X X - - = 0xfc
         * 
         * ID record:
         * flux:   XXXX-X-X-XXXXXX- = 0xf57e
         * clock:  X X - - - X X X  = 0xc7
         * data:    X X X X X X X - = 0xfe
         * 
         * ID record:
         * flux:   XXXX-X-X-XX-XXXX = 0xf56f
         * clock:  X X - - - X X X  = 0xc7
         * data:    X X X X X - X X = 0xfe
         */
         
        if ((inputfifo == 0xf77a) || (inputfifo == 0xf57e) || (inputfifo == 0xf56f))
        {
            if (reading)
				add_record(records, recordstart, outputbuffer);
			recordstart = cursor - 16;

            outputbuffer.resize(1);
            *outputbuffer.begin() = extract_data_bits(inputfifo);

            reading = true;
            bitcount = 0;
            phase = false;
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

