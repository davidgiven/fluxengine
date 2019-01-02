#include "globals.h"
#include "fluxmap.h"
#include "protocol.h"
#include "record.h"
#include <string.h>
#include <algorithm>

#define CLOCK_LOCK_BOOST 6 /* arbitrary */
#define CLOCK_LOCK_DECAY 1 /* arbitrary */
#define CLOCK_DETECTOR_AMPLITUDE_THRESHOLD 60 /* arbi4rary */
#define CLOCK_ERROR_BOUNDS 0.25

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

RecordVector decodeBitsToRecordsMfm(const std::vector<bool>& bits)
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
         * The IAM record, which is the first one on the disk (and is optional), uses
         * a distorted 0xC2 0xC2 0xC2 marker to identify it. Unfortunately, if this is
         * shifted out of phase, it becomes a legal encoding, so if we're looking at
         * real data we can't honour this.
         * 
         * 0xC2 is:
         * data:    1  1  0  0  0  0  1 0
         * mfm:     01 01 00 10 10 10 01 00 = 0x5254
         * special: 01 01 00 10 00 10 01 00 = 0x5224
         *                    ^^^^
         * shifted: 10 10 01 00 01 00 10 0. = legal, and might happen in real data
         * 
         * Therefore, when we've read the marker, the input fifo will contain
         * 0xXXXX522252225222.
         * 
         * All other records use 0xA1 as a marker:
         * 
         * 0xA1  is:
         * data:    1  0  1  0  0  0  0  1
         * mfm:     01 00 01 00 10 10 10 01 = 0x44A9
         * special: 01 00 01 00 10 00 10 01 = 0x4489
         *                       ^^^^^
         * shifted: 10 00 10 01 00 01 00 1
         * 
         * When this is shifted out of phase, we get an illegal encoding (you
         * can't do 10 00). So, if we ever see 0x448944894489 in the input
         * fifo, we know we've landed at the beginning of a new record.
         */
         
        uint64_t masked = inputfifo & 0xFFFFFFFFFFFFLL;
        if ((!reading && (masked == 0x522452245224LL)) || (masked == 0x448944894489LL))
        {
            if (reading)
				add_record(records, recordstart, outputbuffer);
			recordstart = cursor - 4*3*8;

            outputbuffer.resize(3);
            std::fill(outputbuffer.begin(), outputbuffer.begin()+3, reading ? 0xA1 : 0xC2);

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
