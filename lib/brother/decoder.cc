#include "globals.h"
#include "sql.h"
#include "fluxmap.h"
#include "decoders.h"
#include "record.h"
#include "brother.h"
#include <ctype.h>

static std::vector<uint8_t> outputbuffer;

/*
 * Brother disks have this very very non-IBM system where sector header records
 * and data records use two different kinds of GCR: sector headers are 8-in-16
 * (but the encodable values range from 0 to 77ish only) and data headers are
 * 5-in-8. In addition, there's a non-encoded 10-bit ID word at the beginning
 * of each record, as well as a string of 53 1s introducing them. That does at
 * least make them easy to find.
 *
 * Disk formats vary from machine to machine, but mine uses 78 tracks. Track 0
 * is erased but not formatted.  Track alignment is extremely dubious and
 * Brother track 0 shows up on my machine at track 2.
 */

static int decode_data_gcr(uint8_t gcr)
{
    switch (gcr)
    {
		#define GCR_ENTRY(gcr, data) \
			case gcr: return data;
		#include "data_gcr.h"
		#undef GCR_ENTRY
    }
    return -1;
};

static int decode_header_gcr(uint16_t word)
{
	switch (word)
	{
		#define GCR_ENTRY(gcr, data) \
			case gcr: return data;
		#include "header_gcr.h"
		#undef GCR_ENTRY
	}                       
	return -1;             
};

static void add_record(RecordVector& records,
	nanoseconds_t position, const std::vector<uint8_t>& data)
{
	records.push_back(std::unique_ptr<Record>(new Record(position, data)));
}

RecordVector BrotherBitmapDecoder::decodeBitsToRecords(const std::vector<bool>& bits)
{
    RecordVector records;

	enum
	{
		SEEKING,
		READINGSECTOR,
		READINGDATA
	};

    size_t cursor = 0;
    uint32_t inputfifo = 0;
	int inputcount = 0;
	uint8_t outputfifo = 0;
	int outputcount = 0;
    int state = SEEKING;
	size_t recordstart = 0;

    while (cursor < bits.size())
    {
        bool bit = bits[cursor++];
        inputfifo = (inputfifo << 1) | bit;
		inputcount++;

		if (inputfifo == BROTHER_SECTOR_RECORD)
		{
			if (state != SEEKING)
				add_record(records, recordstart, outputbuffer);
			outputbuffer.resize(1);
			outputbuffer[0] = BROTHER_SECTOR_RECORD & 0xff;
			state = READINGSECTOR;
			inputcount = 0;
			recordstart = cursor - 1;
		}
		else if (inputfifo == BROTHER_DATA_RECORD)
		{
			if (state != SEEKING)
				add_record(records, recordstart, outputbuffer);
			outputbuffer.resize(1);
			outputbuffer[0] = BROTHER_DATA_RECORD & 0xff;
			state = READINGDATA;
			outputcount = 0;
			inputcount = 0;
			recordstart = cursor - 1;
		}
		else
		{
			switch (state)
			{
				case READINGSECTOR:
				{
					if (inputcount != 16)
						break;
					int data = decode_header_gcr(inputfifo & 0xffff);
					inputcount = 0;

					outputbuffer.push_back(data);
					break;
				}

				case READINGDATA:
				{
					if (inputcount != 8)
						break;
					int data = decode_data_gcr(inputfifo & 0xff);
					inputcount = 0;

					for (int i=0; i<5; i++)
					{
						outputfifo = (outputfifo << 1) | !!(data & 0x10);
						data <<= 1;
						outputcount++;
						if (outputcount == 8)
						{
							outputbuffer.push_back(outputfifo);
							outputcount = 0;
						}
					}
					break;
				}
			}
		}
    }

	if (state != SEEKING)
		add_record(records, recordstart, outputbuffer);

    return records;
}

